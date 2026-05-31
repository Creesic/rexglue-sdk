// FH1 codegen patcher — see patcher.h for design rationale.
//
// Implementation notes:
//   - Function-body scoping uses brace counting with C/C++ string and char
//     literal awareness. Codegen output is well-formed C++; we don't need to
//     handle macros that produce unbalanced braces because rexglue doesn't
//     emit any.
//   - The match step is exact substring, not regex. Anchors must be precise.
//     We deliberately reject ambiguous matches (>1 occurrence) so a patch
//     never silently rewrites the wrong site.

#include "patcher.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include <fmt/format.h>
#include <rex/logging.h>

namespace fh1::patcher {

namespace {

std::string read_file(const std::filesystem::path& p) {
    std::ifstream is(p, std::ios::binary);
    if (!is) return {};
    std::ostringstream ss;
    ss << is.rdbuf();
    return ss.str();
}

bool write_file(const std::filesystem::path& p, std::string_view content) {
    std::ofstream os(p, std::ios::binary | std::ios::trunc);
    if (!os) return false;
    os.write(content.data(), static_cast<std::streamsize>(content.size()));
    return os.good();
}

// Skip a C/C++ comment or string starting at `i`. Returns the index after
// the terminator. If `s[i]` is not the start of a comment or string, returns
// `i` unchanged.
size_t skip_lexeme(std::string_view s, size_t i) {
    if (i + 1 >= s.size()) return i;
    if (s[i] == '/' && s[i + 1] == '/') {
        // Line comment — to next newline.
        auto nl = s.find('\n', i + 2);
        return nl == std::string_view::npos ? s.size() : nl + 1;
    }
    if (s[i] == '/' && s[i + 1] == '*') {
        auto end = s.find("*/", i + 2);
        return end == std::string_view::npos ? s.size() : end + 2;
    }
    if (s[i] == '"' || s[i] == '\'') {
        char q = s[i];
        size_t j = i + 1;
        while (j < s.size()) {
            if (s[j] == '\\' && j + 1 < s.size()) { j += 2; continue; }
            if (s[j] == q) return j + 1;
            ++j;
        }
        return s.size();
    }
    return i;
}

// Find the function body span (start = index AFTER opening `{`, end = index
// of matching closing `}`) for `DEFINE_REX_FUNC(<function>) {`.
// Returns {npos, npos} if not found.
struct FunctionSpan { size_t body_start; size_t body_end; };

FunctionSpan find_function_body(std::string_view src, std::string_view function) {
    constexpr auto npos = std::string_view::npos;
    std::string header = fmt::format("DEFINE_REX_FUNC({}) {{", function);
    auto hpos = src.find(header);
    if (hpos == npos) return {npos, npos};
    size_t body_start = hpos + header.size();
    // Scan forward from body_start with brace counting + lexeme skipping.
    int depth = 1;
    size_t i = body_start;
    while (i < src.size()) {
        size_t skipped = skip_lexeme(src, i);
        if (skipped != i) { i = skipped; continue; }
        char c = src[i];
        if (c == '{') ++depth;
        else if (c == '}') {
            if (--depth == 0) return {body_start, i};
        }
        ++i;
    }
    return {npos, npos};
}

size_t count_occurrences(std::string_view scope, std::string_view needle) {
    if (needle.empty()) return 0;
    size_t count = 0;
    size_t pos = 0;
    while ((pos = scope.find(needle, pos)) != std::string_view::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

}  // namespace

ApplyOutcome apply_one(const Patch& patch, const std::filesystem::path& codegen_dir) {
    ApplyOutcome r;
    r.patch_id = std::string(patch.id);

    auto target = codegen_dir / std::string(patch.file);
    if (!std::filesystem::exists(target)) {
        r.kind = ApplyOutcome::Kind::FileMissing;
        r.detail = target.string();
        return r;
    }

    std::string src = read_file(target);
    if (src.empty()) {
        r.kind = ApplyOutcome::Kind::FileMissing;
        r.detail = "empty or unreadable: " + target.string();
        return r;
    }

    auto [body_start, body_end] = find_function_body(src, patch.function);
    if (body_start == std::string_view::npos) {
        r.kind = ApplyOutcome::Kind::FunctionMissing;
        r.detail = std::string(patch.function);
        return r;
    }
    std::string_view body(src.data() + body_start, body_end - body_start);

    // Idempotency: marker present inside this function's body → already applied.
    if (body.find(patch.marker) != std::string_view::npos) {
        r.kind = ApplyOutcome::Kind::AlreadyApplied;
        r.detail = std::string(patch.marker);
        return r;
    }

    size_t hits = count_occurrences(body, patch.match);
    if (hits == 0) {
        r.kind = ApplyOutcome::Kind::AnchorMissing;
        r.detail = fmt::format("function='{}' match length={} bytes",
                               patch.function, patch.match.size());
        return r;
    }
    if (hits > 1) {
        r.kind = ApplyOutcome::Kind::AnchorAmbiguous;
        r.detail = fmt::format("function='{}' match appeared {} times", patch.function, hits);
        return r;
    }

    // Single match. Replace.
    auto match_off_in_body = body.find(patch.match);
    size_t abs_off = body_start + match_off_in_body;
    std::string out;
    out.reserve(src.size() + patch.with.size());
    out.append(src, 0, abs_off);
    out.append(patch.with);
    out.append(src, abs_off + patch.match.size());

    if (!write_file(target, out)) {
        r.kind = ApplyOutcome::Kind::AnchorMissing;
        r.detail = "write failed: " + target.string();
        return r;
    }
    r.kind = ApplyOutcome::Kind::Applied;
    r.detail = fmt::format("file='{}' function='{}'", patch.file, patch.function);
    return r;
}

ApplyReport apply_all(const std::filesystem::path& codegen_dir) {
    ApplyReport report;
    for (const auto& p : kPatches) {
        auto out = apply_one(p, codegen_dir);
        switch (out.kind) {
            case ApplyOutcome::Kind::Applied:
                ++report.applied;
                REXLOG_INFO("[fh1-patcher] {} APPLIED — {}", out.patch_id, out.detail);
                break;
            case ApplyOutcome::Kind::AlreadyApplied:
                ++report.already_applied;
                REXLOG_INFO("[fh1-patcher] {} SKIP (already applied)", out.patch_id);
                break;
            case ApplyOutcome::Kind::AnchorMissing:
            case ApplyOutcome::Kind::AnchorAmbiguous:
            case ApplyOutcome::Kind::FunctionMissing:
            case ApplyOutcome::Kind::FileMissing:
                ++report.failed;
                REXLOG_WARN("[fh1-patcher] {} FAIL ({}) — {}",
                            out.patch_id,
                            out.kind == ApplyOutcome::Kind::AnchorMissing      ? "anchor missing"
                            : out.kind == ApplyOutcome::Kind::AnchorAmbiguous  ? "anchor ambiguous"
                            : out.kind == ApplyOutcome::Kind::FunctionMissing  ? "function missing"
                                                                               : "file missing",
                            out.detail);
                break;
        }
        report.outcomes.push_back(std::move(out));
    }
    REXLOG_INFO("[fh1-patcher] done: {} applied, {} already-applied, {} failed (of {} total)",
                report.applied, report.already_applied, report.failed,
                static_cast<int>(kPatches.size()));
    return report;
}

std::optional<ApplyReport> apply_all_if_present(const std::filesystem::path& codegen_dir) {
    if (!std::filesystem::is_directory(codegen_dir)) return std::nullopt;
    // Cheap probe: at least one of our targets must exist. If none do,
    // the binary likely shipped with codegen baked in — skip silently
    // rather than emitting 11 noisy "file missing" warnings.
    bool any = false;
    for (const auto& p : kPatches) {
        if (std::filesystem::exists(codegen_dir / std::string(p.file))) {
            any = true;
            break;
        }
    }
    if (!any) return std::nullopt;
    return apply_all(codegen_dir);
}

}  // namespace fh1::patcher

// Pull the patch table into this TU. patches.inc defines kPatches.
#include "patches.inc"
