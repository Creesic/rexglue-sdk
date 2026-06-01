/**
 * @file        codegen/manifest.cpp
 * @brief       Manifest TOML parser for multi-binary projects
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/codegen/manifest.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <toml++/toml.hpp>

#include <fmt/format.h>
#include <rex/logging.h>
#include <rex/system/guest_path.h>

namespace rex::codegen {

std::string CanonicalizeModuleGuestPath(std::string_view path, std::string_view project_name) {
  std::string guest_path = rex::system::NormalizeGuestPath(path);

  if (!project_name.empty()) {
    std::string lower_project(project_name);
    std::transform(lower_project.begin(), lower_project.end(), lower_project.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const std::string project_assets_prefix = lower_project + "/assets/";
    if (guest_path.rfind(project_assets_prefix, 0) == 0) {
      guest_path.erase(0, project_assets_prefix.size());
    }
  }

  return guest_path;
}

namespace {

bool IsValidProjectName(std::string_view name) {
  if (name.empty())
    return false;
  if (!(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_')) {
    return false;
  }
  return std::all_of(name.begin(), name.end(),
                     [](unsigned char c) { return std::isalnum(c) || c == '_'; });
}

/**
 * Pull file_path / out_directory_path / project_name onto the recompiler
 * config so downstream consumers can rely on them. Other fields (codegen
 * flags, sub-tables, includes) come from RecompilerConfig::LoadFromTable.
 */
bool LoadBinaryConfig(const toml::table& tbl, const std::filesystem::path& base_dir,
                      std::string_view project_name, BinaryConfig& out) {
  out.recompiler = RecompilerConfig{};
  out.recompiler.projectName = std::string(project_name);
  if (!out.recompiler.LoadFromTable(tbl, base_dir)) {
    return false;
  }
  return true;
}

std::optional<uint32_t> ParseStubSymbolAddress(std::string_view symbol) {
  if (!symbol.starts_with("sub_")) {
    return std::nullopt;
  }
  symbol.remove_prefix(4);
  if (symbol.starts_with("0x") || symbol.starts_with("0X")) {
    symbol.remove_prefix(2);
  }
  if (symbol.empty()) {
    return std::nullopt;
  }
  try {
    return static_cast<uint32_t>(std::stoul(std::string(symbol), nullptr, 16));
  } catch (...) {
    return std::nullopt;
  }
}

std::string_view TrimLine(std::string_view line) {
  while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) {
    line.remove_prefix(1);
  }
  while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) {
    line.remove_suffix(1);
  }
  return line;
}

bool StartsWithDirective(std::string_view line, std::string_view directive) {
  return line.size() >= directive.size() && line.substr(0, directive.size()) == directive;
}

std::optional<std::string> ExpandStubDirectiveLine(std::string_view line) {
  const auto hash = line.find('#');
  const std::string_view code = TrimLine(line.substr(0, hash));
  const std::string_view comment = hash == std::string_view::npos ? std::string_view{} : line.substr(hash);

  auto emit = [&](uint32_t address, std::optional<int64_t> stub_return) -> std::string {
    std::string out;
    if (stub_return.has_value()) {
      out = fmt::format("0x{:08X} = {{ stub = true, stub_return = {} }}", address, *stub_return);
    } else {
      out = fmt::format("0x{:08X} = {{ stub = true }}", address);
    }
    if (!comment.empty()) {
      if (!comment.empty() && comment.front() != ' ') {
        out += ' ';
      }
      out += comment;
    }
    return out;
  };

  auto parse_call = [&](std::string_view prefix, bool with_return) -> std::optional<std::string> {
    if (!StartsWithDirective(code, prefix) || code.back() != ')') {
      return std::nullopt;
    }
    std::string_view args = code.substr(prefix.size(), code.size() - prefix.size() - 1);
    args = TrimLine(args);
    if (!with_return) {
      if (auto addr = ParseStubSymbolAddress(args)) {
        return emit(*addr, std::nullopt);
      }
      return std::nullopt;
    }
    const auto comma = args.find(',');
    if (comma == std::string_view::npos) {
      return std::nullopt;
    }
    const auto symbol = TrimLine(args.substr(0, comma));
    const auto value_text = TrimLine(args.substr(comma + 1));
    if (value_text.empty()) {
      return std::nullopt;
    }
    int64_t value = 0;
    try {
      if (value_text.starts_with("0x") || value_text.starts_with("0X")) {
        value = static_cast<int64_t>(std::stoll(std::string(value_text.substr(2)), nullptr, 16));
      } else {
        value = std::stoll(std::string(value_text), nullptr, 0);
      }
    } catch (...) {
      return std::nullopt;
    }
    if (auto addr = ParseStubSymbolAddress(symbol)) {
      return emit(*addr, value);
    }
    return std::nullopt;
  };

  if (auto expanded = parse_call("REX_STUB_RETURN(", true)) {
    return expanded;
  }
  if (auto expanded = parse_call("PPC_STUB_RETURN(", true)) {
    return expanded;
  }
  if (auto expanded = parse_call("REX_STUB(", false)) {
    return expanded;
  }
  if (auto expanded = parse_call("PPC_STUB(", false)) {
    return expanded;
  }
  return std::nullopt;
}

std::string PreprocessTomlContent(std::string_view content) {
  std::ostringstream out;
  size_t start = 0;
  bool first = true;
  while (start <= content.size()) {
    const size_t end = content.find('\n', start);
    const size_t line_end = end == std::string_view::npos ? content.size() : end;
    const std::string_view line = content.substr(start, line_end - start);
    if (!first) {
      out << '\n';
    }
    first = false;
    if (auto expanded = ExpandStubDirectiveLine(line)) {
      out << *expanded;
    } else {
      out << line;
    }
    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }
  if (!content.empty() && content.back() == '\n') {
    out << '\n';
  }
  return out.str();
}

}  // namespace

std::string ManifestConfig::PreprocessTomlDirectives(std::string_view content) {
  return PreprocessTomlContent(content);
}

std::optional<toml::table> ManifestConfig::ParseTomlFilePreprocessed(
    const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    REXLOG_ERROR("Failed to open TOML file: {}", path.string());
    return std::nullopt;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string processed = PreprocessTomlDirectives(ss.str());
  try {
    return toml::parse(processed, path.string());
  } catch (const toml::parse_error& err) {
    REXLOG_ERROR("Failed to parse {}: {}", path.string(), err.what());
    return std::nullopt;
  }
}

std::optional<ManifestConfig> ManifestConfig::Load(const std::filesystem::path& path) {
  auto parsed = ParseTomlFilePreprocessed(path);
  if (!parsed) {
    return std::nullopt;
  }
  const toml::table& tbl = *parsed;

  ManifestConfig manifest;
  manifest.manifestDir = path.parent_path();

  auto* project = tbl["project"].as_table();
  if (!project) {
    REXLOG_ERROR("Manifest missing [project] section: {}", path.string());
    return std::nullopt;
  }
  manifest.projectName = (*project)["name"].value_or<std::string>("");
  if (manifest.projectName.empty()) {
    REXLOG_ERROR("Manifest missing [project].name: {}", path.string());
    return std::nullopt;
  }
  if (!IsValidProjectName(manifest.projectName)) {
    REXLOG_ERROR(
        "Manifest [project].name '{}' is not a valid identifier "
        "(letters, digits, underscore; must not start with a digit): {}",
        manifest.projectName, path.string());
    return std::nullopt;
  }
  if (auto stamp = (*project)["sdk_version"].value<std::string>(); stamp && !stamp->empty()) {
    manifest.sdkVersion = *stamp;
  }
  if (auto root = (*project)["game_root"].value<std::string>(); root && !root->empty()) {
    manifest.gameRoot = *root;
  }

  auto* entrypoint = tbl["entrypoint"].as_table();
  if (!entrypoint) {
    REXLOG_ERROR("Manifest missing [entrypoint] section: {}", path.string());
    return std::nullopt;
  }
  if (!LoadBinaryConfig(*entrypoint, manifest.manifestDir, manifest.projectName,
                        manifest.entrypoint)) {
    return std::nullopt;
  }

  if (auto* functions = tbl["functions"].as_table(); functions && !functions->empty()) {
    toml::table overlay;
    overlay.insert("functions", *functions);
    if (!manifest.entrypoint.recompiler.LoadFromTable(overlay, manifest.manifestDir)) {
      return std::nullopt;
    }
  }

  if (auto modules = tbl["modules"].as_array()) {
    size_t index = 0;
    for (const auto& mod : *modules) {
      auto* modTbl = mod.as_table();
      if (!modTbl) {
        REXLOG_ERROR("Manifest [[modules]] entry #{} is not a table", index);
        return std::nullopt;
      }
      BinaryConfig binary;
      if (!LoadBinaryConfig(*modTbl, manifest.manifestDir, manifest.projectName, binary)) {
        return std::nullopt;
      }
      auto guest_path = (*modTbl)["guest_path"].value_or<std::string>("");
      if (guest_path.empty()) {
        REXLOG_ERROR("Manifest [[modules]] entry #{} missing guest_path", index);
        return std::nullopt;
      }
      binary.guestPath = CanonicalizeModuleGuestPath(guest_path, manifest.projectName);
      for (const auto& existing : manifest.modules) {
        if (existing.guestPath == binary.guestPath) {
          REXLOG_ERROR("Manifest [[modules]] duplicate guest_path '{}' (entry #{})",
                       binary.guestPath, index);
          return std::nullopt;
        }
      }
      manifest.modules.push_back(std::move(binary));
      ++index;
    }
  }

  return manifest;
}

bool ManifestConfig::IsManifest(const std::filesystem::path& path) {
  auto tbl = ParseTomlFilePreprocessed(path);
  return tbl.has_value() && tbl->contains("project");
}

namespace {

std::optional<std::string> ParseSectionHeader(std::string_view line) {
  auto first = line.find_first_not_of(" \t");
  if (first == std::string_view::npos)
    return std::nullopt;
  if (line[first] != '[' || (first + 1 < line.size() && line[first + 1] == '['))
    return std::nullopt;
  auto end = line.find(']', first + 1);
  if (end == std::string_view::npos)
    return std::nullopt;
  return std::string(line.substr(first + 1, end - first - 1));
}

bool LineSetsKey(std::string_view line, std::string_view key) {
  auto first = line.find_first_not_of(" \t");
  if (first == std::string_view::npos)
    return false;
  if (line.compare(first, key.size(), key) != 0)
    return false;
  auto after = first + key.size();
  while (after < line.size() && (line[after] == ' ' || line[after] == '\t'))
    ++after;
  return after < line.size() && line[after] == '=';
}

}  // namespace

bool ManifestConfig::WriteSdkVersionStamp(const std::filesystem::path& path,
                                          std::string_view version) {
  std::ifstream in(path);
  if (!in) {
    REXLOG_ERROR("Failed to open manifest for stamping: {}", path.string());
    return false;
  }
  std::vector<std::string> lines;
  std::string buf;
  while (std::getline(in, buf)) {
    lines.push_back(std::move(buf));
    buf.clear();
  }
  in.close();

  const std::string stamp_line = "sdk_version = \"" + std::string(version) + "\"";

  std::optional<size_t> project_header_idx;
  std::optional<size_t> stamp_idx;
  bool in_project = false;
  for (size_t i = 0; i < lines.size(); ++i) {
    auto sec = ParseSectionHeader(lines[i]);
    if (sec) {
      in_project = (*sec == "project");
      if (in_project) {
        project_header_idx = i;
        stamp_idx.reset();
      }
      continue;
    }
    if (in_project && !stamp_idx && LineSetsKey(lines[i], "sdk_version")) {
      stamp_idx = i;
    }
  }

  if (stamp_idx) {
    lines[*stamp_idx] = stamp_line;
  } else if (project_header_idx) {
    lines.insert(lines.begin() + *project_header_idx + 1, stamp_line);
  } else {
    if (!lines.empty() && !lines.back().empty())
      lines.emplace_back();
    lines.emplace_back("[project]");
    lines.push_back(stamp_line);
  }

  auto tmp_path = path;
  tmp_path += ".tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary);
    if (!out) {
      REXLOG_ERROR("Failed to open manifest tmp for writing: {}", tmp_path.string());
      return false;
    }
    for (size_t i = 0; i < lines.size(); ++i) {
      out << lines[i];
      if (i + 1 < lines.size())
        out << '\n';
    }
    out << '\n';
    if (!out.good()) {
      REXLOG_ERROR("Failed while writing manifest tmp: {}", tmp_path.string());
      std::error_code ignore;
      std::filesystem::remove(tmp_path, ignore);
      return false;
    }
  }

  std::error_code ec;
  std::filesystem::rename(tmp_path, path, ec);
  if (ec) {
    REXLOG_ERROR("Failed to rename manifest tmp into place: {}", ec.message());
    std::error_code ignore;
    std::filesystem::remove(tmp_path, ignore);
    return false;
  }
  return true;
}

}  // namespace rex::codegen
