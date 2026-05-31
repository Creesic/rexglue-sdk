// FH1 codegen patcher — anchor-based, idempotent.
//
// Why this exists: rexglue codegen output (run-fh1/out-v4/*.cpp) is
// regenerated whenever the user re-runs codegen against their XEX. Every
// regen wipes any loose .patch files we'd previously applied. Line-number
// unified diffs (patch -p1) are fragile — small lifter changes shift offsets
// and the patch rejects.
//
// This patcher addresses both:
//   - Idempotent: each patch has a unique `marker` string. Before applying,
//     we scan the target file; if the marker already appears, we skip.
//     Re-running apply_all() after a fresh regen converges to the patched
//     state without duplicating insertions.
//   - Anchor-based: a patch is identified by (function name, instruction
//     sequence to match), not by file line number. As long as the anchor
//     sequence still appears in the function body, the patch applies.
//
// The applet's install/upgrade flow calls apply_all() after codegen runs
// (or after a user-requested rebuild). The patcher reads and writes the
// codegen .cpp files in-place. Failures don't abort the build — they're
// collected and reported so the user can manually fix anchor drift.
//
// All patch data lives in patches.inc (one Patch entry per former .patch
// file). To add a new patch, drop a new entry in patches.inc.

#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace fh1::patcher {

struct Patch {
    // Sequential id for log lines and diagnostics ("0004", "0008", ...).
    std::string_view id;

    // Path relative to the codegen output directory, e.g. "fh1_recomp.87.cpp".
    std::string_view file;

    // Name of the DEFINE_REX_FUNC body the patch lives inside. The patcher
    // restricts both the marker scan and the anchor search to this function
    // body — guards against accidental cross-function collisions.
    std::string_view function;

    // Unique substring whose presence in the target file means "already
    // applied; skip". Conventionally the unique static atomic counter name
    // the patch declares (e.g. "fh1_updateislands_null_evt_") since it's
    // guaranteed unique across patches and absent from clean codegen.
    std::string_view marker;

    // The exact substring to find inside the function body. Must match
    // exactly once. Leading/trailing whitespace is preserved verbatim;
    // include the tabs the codegen emits.
    std::string_view match;

    // Replacement substring written in place of `match`.
    std::string_view with;
};

struct ApplyOutcome {
    enum class Kind { Applied, AlreadyApplied, AnchorMissing, AnchorAmbiguous, FunctionMissing, FileMissing };
    Kind kind = Kind::Applied;
    std::string detail;     // free-form context (counts, surrounding text)
    std::string patch_id;
};

struct ApplyReport {
    int applied = 0;
    int already_applied = 0;
    int failed = 0;
    std::vector<ApplyOutcome> outcomes;  // one entry per patch attempted
};

// The full patch table. Defined in patches.inc and exposed via this span.
extern const std::span<const Patch> kPatches;

// Apply every patch in kPatches against files under codegen_dir.
// `codegen_dir` is typically rexglue/run-fh1/out-v4/. Idempotent — patches
// already present (marker found) are reported as AlreadyApplied.
ApplyReport apply_all(const std::filesystem::path& codegen_dir);

// Apply one specific patch. Same semantics; useful for targeted testing.
ApplyOutcome apply_one(const Patch& patch, const std::filesystem::path& codegen_dir);

// Convenience for the install applet: apply_all() iff codegen_dir exists
// AND contains at least one of our target files. Returns std::nullopt
// (empty report optional) when there's nothing to do; otherwise the full
// report. Use this from installer flows that may run on systems without
// codegen output present (e.g., shipped binaries with codegen already baked
// in — calling apply_all there would just no-op every entry as missing).
std::optional<ApplyReport> apply_all_if_present(const std::filesystem::path& codegen_dir);

}  // namespace fh1::patcher
