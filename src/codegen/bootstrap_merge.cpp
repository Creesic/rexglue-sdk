/**
 * @file        codegen/bootstrap_merge.cpp
 * @brief       Merge bootstrap-discovered function stubs into config/manifest TOML
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/codegen/bootstrap_merge.h>

#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <cctype>
#include <fmt/format.h>
#include <system_error>
#include <toml++/toml.hpp>

#include <rex/logging.h>

#include "codegen_logging.h"

namespace rex::codegen {

namespace {

bool WriteAllLinesAtomically(const std::filesystem::path& path,
                             const std::vector<std::string>& lines);

uint32_t ParseGuestAddressKey(std::string_view key) {
  if (key.size() < 3 || key[0] != '0')
    return 0;
  if (key[1] != 'x' && key[1] != 'X')
    return 0;
  try {
    return static_cast<uint32_t>(std::stoul(std::string(key), nullptr, 16));
  } catch (...) {
    return 0;
  }
}

std::set<uint32_t> LoadFunctionsTable(const toml::table* functions) {
  std::set<uint32_t> addresses;
  if (!functions) {
    return addresses;
  }
  for (const auto& [key, node] : *functions) {
    if (uint32_t addr = ParseGuestAddressKey(key.str()); addr != 0) {
      addresses.insert(addr);
    }
  }
  return addresses;
}

bool TryParseBootstrapAddressLine(std::string_view line, uint32_t* out_address) {
  if (!out_address) {
    return false;
  }
  auto first = line.find_first_not_of(" \t");
  if (first == std::string_view::npos || line[first] != '0') {
    return false;
  }
  if (first + 10 > line.size() || (line[first + 1] != 'x' && line[first + 1] != 'X')) {
    return false;
  }
  const std::string hex(line.substr(first + 2, 8));
  for (char c : hex) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) {
      return false;
    }
  }
  try {
    *out_address = static_cast<uint32_t>(std::stoul(hex, nullptr, 16));
    return true;
  } catch (...) {
    return false;
  }
}

std::set<uint32_t> ScanBootstrapAddressesFromFile(const std::filesystem::path& path) {
  std::set<uint32_t> addresses;
  if (path.empty() || !std::filesystem::exists(path)) {
    return addresses;
  }

  std::ifstream in(path);
  if (!in) {
    return addresses;
  }

  std::string line;
  while (std::getline(in, line)) {
    uint32_t addr = 0;
    if (TryParseBootstrapAddressLine(line, &addr)) {
      addresses.insert(addr);
    }
  }
  return addresses;
}

size_t CountBootstrapAddressLines(const std::filesystem::path& path) {
  if (path.empty() || !std::filesystem::exists(path)) {
    return 0;
  }

  std::ifstream in(path);
  if (!in) {
    return 0;
  }

  size_t count = 0;
  std::string line;
  while (std::getline(in, line)) {
    uint32_t addr = 0;
    if (TryParseBootstrapAddressLine(line, &addr)) {
      ++count;
    }
  }
  return count;
}

bool WriteBootstrapFunctionsToml(const std::filesystem::path& path,
                                 const std::set<uint32_t>& addresses) {
  std::vector<std::string> lines;
  lines.emplace_back("# Auto-discovered guest functions (bootstrap mode)");
  lines.emplace_back("[functions]");
  for (uint32_t addr : addresses) {
    lines.push_back(fmt::format("0x{:08X} = {{}}", addr));
  }
  return WriteAllLinesAtomically(path, lines);
}

bool CompactBootstrapFunctionsToml(const std::filesystem::path& path) {
  if (path.empty() || !std::filesystem::exists(path)) {
    return false;
  }

  const std::set<uint32_t> addresses = ScanBootstrapAddressesFromFile(path);
  if (addresses.empty()) {
    return false;
  }

  const size_t line_count = CountBootstrapAddressLines(path);
  if (line_count <= addresses.size()) {
    return false;
  }

  if (!WriteBootstrapFunctionsToml(path, addresses)) {
    REXCODEGEN_WARN("Bootstrap merge: failed to compact {}", path.string());
    return false;
  }

  REXCODEGEN_INFO("Bootstrap merge: compacted {} ({} duplicate line(s) removed)", path.string(),
                  line_count - addresses.size());
  return true;
}

std::set<uint32_t> LoadAddressesFromTomlFile(const std::filesystem::path& path) {
  if (path.empty() || !std::filesystem::exists(path)) {
    return {};
  }

  try {
    const toml::table tbl = toml::parse_file(path.string());
    return LoadFunctionsTable(tbl["functions"].as_table());
  } catch (const toml::parse_error& err) {
    auto scanned = ScanBootstrapAddressesFromFile(path);
    if (!scanned.empty()) {
      REXCODEGEN_WARN("Bootstrap merge: {} has duplicate/invalid TOML; loaded {} unique address(es) "
                      "via line scan ({})",
                      path.string(), scanned.size(), err.what());
      CompactBootstrapFunctionsToml(path);
      return scanned;
    }
    REXCODEGEN_WARN("Bootstrap merge: failed to parse {}: {}", path.string(), err.what());
  }

  return {};
}

std::set<uint32_t> CollectMergedAddresses(const std::filesystem::path& discovered_path,
                                          const std::filesystem::path& suggestions_path,
                                          const std::set<uint32_t>& emit_suggestions,
                                          uint32_t image_base, uint32_t image_size,
                                          const std::set<uint32_t>& ignored_addresses) {
  std::set<uint32_t> merged = emit_suggestions;
  for (uint32_t addr : LoadAddressesFromTomlFile(discovered_path)) {
    merged.insert(addr);
  }
  for (uint32_t addr : LoadAddressesFromTomlFile(suggestions_path)) {
    merged.insert(addr);
  }
  merged = FilterBootstrapAddressesInImage(merged, image_base, image_size);
  size_t ignored_count = 0;
  for (uint32_t addr : ignored_addresses) {
    ignored_count += merged.erase(addr);
  }
  if (ignored_count > 0) {
    REXCODEGEN_WARN("Bootstrap merge: ignored {} discovered/suggested function(s)", ignored_count);
  }
  return merged;
}

std::optional<std::string> ParseSectionHeader(std::string_view line) {
  auto first = line.find_first_not_of(" \t");
  if (first == std::string_view::npos) {
    return std::nullopt;
  }
  if (line[first] != '[' || (first + 1 < line.size() && line[first + 1] == '[')) {
    return std::nullopt;
  }
  auto end = line.find(']', first + 1);
  if (end == std::string_view::npos) {
    return std::nullopt;
  }
  return std::string(line.substr(first + 1, end - first - 1));
}

std::string FunctionsSectionName(std::string_view section) {
  return fmt::format("{}.functions", section);
}

bool IsDottedFunctionHeader(std::string_view section, std::string_view header) {
  const std::string prefix = FunctionsSectionName(section) + ".0x";
  return header.size() >= prefix.size() && header.substr(0, prefix.size()) == prefix;
}

bool TryParseDottedFunctionHeader(std::string_view header, uint32_t* out_address) {
  if (!out_address) {
    return false;
  }
  const auto pos = header.rfind("0x");
  if (pos == std::string_view::npos || pos + 10 > header.size()) {
    return false;
  }
  return TryParseBootstrapAddressLine(header.substr(pos), out_address);
}

bool ReadAllLines(const std::filesystem::path& path, std::vector<std::string>* lines) {
  if (!lines) {
    return false;
  }
  std::ifstream in(path);
  if (!in) {
    return false;
  }
  lines->clear();
  std::string buf;
  while (std::getline(in, buf)) {
    lines->push_back(std::move(buf));
    buf.clear();
  }
  return true;
}

bool WriteAllLinesAtomically(const std::filesystem::path& path,
                             const std::vector<std::string>& lines) {
  auto tmp_path = path;
  tmp_path += ".tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary);
    if (!out) {
      return false;
    }
    for (size_t i = 0; i < lines.size(); ++i) {
      out << lines[i];
      if (i + 1 < lines.size()) {
        out << '\n';
      }
    }
    out << '\n';
  }
  std::error_code ec;
  std::filesystem::rename(tmp_path, path, ec);
  if (ec) {
    std::filesystem::remove(tmp_path, ec);
    return false;
  }
  return true;
}

std::set<uint32_t> ScanManifestFunctionAddresses(const std::filesystem::path& manifest_path,
                                                 std::string_view section) {
  std::set<uint32_t> addresses;
  std::vector<std::string> lines;
  if (!ReadAllLines(manifest_path, &lines)) {
    return addresses;
  }

  const std::string functions_section = FunctionsSectionName(section);
  bool in_functions = false;
  for (const std::string& line : lines) {
    if (auto header = ParseSectionHeader(line)) {
      if (*header == functions_section) {
        in_functions = true;
        continue;
      }
      if (in_functions && IsDottedFunctionHeader(section, *header)) {
        uint32_t addr = 0;
        if (TryParseDottedFunctionHeader(*header, &addr)) {
          addresses.insert(addr);
        }
        continue;
      }
      if (in_functions) {
        in_functions = false;
      }
      continue;
    }

    if (!in_functions) {
      continue;
    }

    uint32_t addr = 0;
    if (TryParseBootstrapAddressLine(line, &addr)) {
      addresses.insert(addr);
    }
  }
  return addresses;
}

size_t RepairAndAppendManifestFunctions(const std::filesystem::path& manifest_path,
                                        std::string_view section,
                                        const std::set<uint32_t>& addresses_to_add) {
  std::vector<std::string> lines;
  if (!ReadAllLines(manifest_path, &lines)) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to read manifest {}", manifest_path.string());
    return 0;
  }

  const std::string functions_section = FunctionsSectionName(section);
  std::set<uint32_t> existing;
  std::string stub_indent = "    ";
  bool in_functions = false;
  bool changed = false;

  std::optional<size_t> functions_header_idx;
  std::optional<size_t> insert_after_idx;

  std::vector<std::string> out_lines;
  out_lines.reserve(lines.size());

  for (const std::string& line : lines) {
    if (auto header = ParseSectionHeader(line)) {
      if (*header == functions_section) {
        in_functions = true;
        functions_header_idx = out_lines.size();
        out_lines.push_back(line);
        continue;
      }

      if (in_functions && IsDottedFunctionHeader(section, *header)) {
        uint32_t addr = 0;
        if (TryParseDottedFunctionHeader(*header, &addr) && !existing.contains(addr)) {
          existing.insert(addr);
          const std::string stub =
              fmt::format("{}0x{:08X} = {}", stub_indent, addr, "{}");
          out_lines.push_back(stub);
          insert_after_idx = out_lines.size() - 1;
          changed = true;
        } else {
          changed = true;
        }
        continue;
      }

      if (in_functions) {
        in_functions = false;
      }
      out_lines.push_back(line);
      continue;
    }

    if (in_functions) {
      uint32_t addr = 0;
      if (TryParseBootstrapAddressLine(line, &addr)) {
        if (existing.contains(addr)) {
          changed = true;
          continue;
        }
        existing.insert(addr);
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && first > 0) {
          stub_indent = line.substr(0, first);
        }
        out_lines.push_back(line);
        insert_after_idx = out_lines.size() - 1;
        continue;
      }
    }

    out_lines.push_back(line);
  }

  const std::set<uint32_t> before_repair = ScanManifestFunctionAddresses(manifest_path, section);
  std::vector<uint32_t> to_insert;
  to_insert.reserve(addresses_to_add.size());
  for (uint32_t addr : addresses_to_add) {
    if (!before_repair.contains(addr)) {
      to_insert.push_back(addr);
    }
  }
  const size_t added = to_insert.size();

  if (added > 0) {
    const size_t insert_at =
        insert_after_idx.value_or(functions_header_idx.value_or(out_lines.size()));
    size_t offset = 0;
    for (uint32_t addr : to_insert) {
      out_lines.insert(out_lines.begin() + static_cast<std::ptrdiff_t>(insert_at + 1 + offset),
                       fmt::format("{}0x{:08X} = {}", stub_indent, addr, "{}"));
      ++offset;
    }
    changed = true;
  }

  if (!changed) {
    return 0;
  }

  if (!WriteAllLinesAtomically(manifest_path, out_lines)) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to write manifest {}", manifest_path.string());
    return 0;
  }

  if (added > 0) {
    REXCODEGEN_INFO("Bootstrap merge: added {} function stub(s) to manifest {} [{}.functions]",
                    added, manifest_path.string(), section);
  } else {
    REXCODEGEN_INFO("Bootstrap merge: repaired function stub formatting in manifest {} "
                    "[{}.functions]",
                    manifest_path.string(), section);
  }
  return added;
}

size_t RepairAndAppendConfigFunctions(const std::filesystem::path& config_path,
                                      const std::set<uint32_t>& addresses_to_add) {
  std::vector<std::string> lines;
  if (!ReadAllLines(config_path, &lines)) {
    if (addresses_to_add.empty()) {
      return 0;
    }
    lines = {"[functions]"};
  }

  constexpr std::string_view functions_section = "functions";
  std::set<uint32_t> existing;
  std::string stub_indent;
  bool in_functions = false;
  bool changed = false;

  std::optional<size_t> functions_header_idx;
  std::optional<size_t> insert_after_idx;

  std::vector<std::string> out_lines;
  out_lines.reserve(lines.size());

  for (const std::string& line : lines) {
    if (auto header = ParseSectionHeader(line)) {
      if (*header == functions_section) {
        in_functions = true;
        functions_header_idx = out_lines.size();
        out_lines.push_back(line);
        continue;
      }

      if (in_functions && header->size() >= 12 && header->substr(0, 12) == "functions.0x") {
        uint32_t addr = 0;
        if (TryParseDottedFunctionHeader(*header, &addr) && !existing.contains(addr)) {
          existing.insert(addr);
          const std::string stub =
              fmt::format("{}0x{:08X} = {}", stub_indent, addr, "{}");
          out_lines.push_back(stub);
          insert_after_idx = out_lines.size() - 1;
          changed = true;
        } else {
          changed = true;
        }
        continue;
      }

      if (in_functions) {
        in_functions = false;
      }
      out_lines.push_back(line);
      continue;
    }

    if (in_functions) {
      uint32_t addr = 0;
      if (TryParseBootstrapAddressLine(line, &addr)) {
        if (existing.contains(addr)) {
          changed = true;
          continue;
        }
        existing.insert(addr);
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && first > 0) {
          stub_indent = line.substr(0, first);
        }
        out_lines.push_back(line);
        insert_after_idx = out_lines.size() - 1;
        continue;
      }
    }

    out_lines.push_back(line);
  }

  if (!functions_header_idx) {
    if (!out_lines.empty() && !out_lines.back().empty()) {
      out_lines.emplace_back();
    }
    out_lines.emplace_back("[functions]");
    functions_header_idx = out_lines.size() - 1;
    changed = true;
  }

  const std::set<uint32_t> before_repair = ScanBootstrapAddressesFromFile(config_path);
  std::vector<uint32_t> to_insert;
  for (uint32_t addr : addresses_to_add) {
    if (!before_repair.contains(addr)) {
      to_insert.push_back(addr);
    }
  }

  const size_t added = to_insert.size();
  if (added > 0) {
    const size_t insert_at =
        insert_after_idx.value_or(functions_header_idx.value_or(out_lines.size()));
    size_t offset = 0;
    for (uint32_t addr : to_insert) {
      out_lines.insert(out_lines.begin() + static_cast<std::ptrdiff_t>(insert_at + 1 + offset),
                       fmt::format("{}0x{:08X} = {}", stub_indent, addr, "{}"));
      ++offset;
    }
    changed = true;
  }

  if (!changed) {
    return 0;
  }

  if (!WriteAllLinesAtomically(config_path, out_lines)) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to write {}", config_path.string());
    return 0;
  }

  if (added > 0) {
    REXCODEGEN_INFO("Bootstrap merge: added {} function stub(s) to {}", added,
                    config_path.string());
  }
  return added;
}

}  // namespace

std::set<uint32_t> LoadBootstrapFunctionAddresses(const std::filesystem::path& path) {
  return LoadAddressesFromTomlFile(path);
}

std::set<uint32_t> LoadManifestFunctions(const std::filesystem::path& manifest_path,
                                         std::string_view section) {
  if (manifest_path.empty() || !std::filesystem::exists(manifest_path)) {
    return {};
  }

  auto scanned = ScanManifestFunctionAddresses(manifest_path, section);
  if (!scanned.empty()) {
    return scanned;
  }

  try {
    const toml::table tbl = toml::parse_file(manifest_path.string());
    const auto* section_tbl = tbl[std::string(section)].as_table();
    if (!section_tbl) {
      return {};
    }
    return LoadFunctionsTable((*section_tbl)["functions"].as_table());
  } catch (const toml::parse_error& err) {
    REXCODEGEN_WARN("Bootstrap merge: failed to parse manifest {}: {}", manifest_path.string(),
                    err.what());
  }
  return {};
}

std::set<uint32_t> FilterBootstrapAddressesInImage(const std::set<uint32_t>& addresses,
                                                   uint32_t image_base, uint32_t image_size) {
  if (image_size == 0) {
    return addresses;
  }

  const uint32_t image_end = image_base + image_size;
  std::set<uint32_t> filtered;
  for (uint32_t addr : addresses) {
    if (addr >= image_base && addr < image_end) {
      filtered.insert(addr);
    } else {
      REXCODEGEN_WARN("Bootstrap: skipping 0x{:08X} (outside image [{:08X}, {:08X}))", addr,
                      image_base, image_end);
    }
  }
  return filtered;
}

void PruneFunctionsOutsideImage(std::unordered_map<uint32_t, FunctionConfig>& functions,
                                uint32_t image_base, uint32_t image_size) {
  if (image_size == 0) {
    return;
  }

  const uint32_t image_end = image_base + image_size;
  for (auto it = functions.begin(); it != functions.end();) {
    if (it->first >= image_base && it->first < image_end) {
      ++it;
      continue;
    }
    REXCODEGEN_WARN("Bootstrap: ignoring configured function 0x{:08X} (outside image [{:08X}, "
                    "{:08X}))",
                    it->first, image_base, image_end);
    it = functions.erase(it);
  }
}

size_t MergeBootstrapStubs(const std::filesystem::path& config_path,
                           const std::set<uint32_t>& addresses) {
  if (config_path.empty()) {
    return 0;
  }
  if (addresses.empty() && std::filesystem::exists(config_path)) {
    return RepairAndAppendConfigFunctions(config_path, {});
  }
  if (addresses.empty()) {
    return 0;
  }
  return RepairAndAppendConfigFunctions(config_path, addresses);
}

size_t MergeBootstrapIntoConfigIncludes(
    const std::filesystem::path& config_dir, const std::vector<std::string>& include_files,
    const std::filesystem::path& discovered_path,
    const std::filesystem::path& suggestions_path, const std::set<uint32_t>& emit_suggestions,
    uint32_t image_base, uint32_t image_size, const std::set<uint32_t>& ignored_addresses) {
  const std::set<uint32_t> merged =
      CollectMergedAddresses(discovered_path, suggestions_path, emit_suggestions, image_base,
                             image_size, ignored_addresses);

  if (merged.empty() || include_files.empty()) {
    return 0;
  }

  size_t total_added = 0;
  for (const auto& include_file : include_files) {
    total_added += MergeBootstrapStubs(config_dir / include_file, merged);
  }
  return total_added;
}

size_t MergeBootstrapIntoManifest(const std::filesystem::path& manifest_path,
                                  std::string_view section,
                                  const std::filesystem::path& discovered_path,
                                  const std::filesystem::path& suggestions_path,
                                  const std::set<uint32_t>& emit_suggestions, uint32_t image_base,
                                  uint32_t image_size, const std::set<uint32_t>& ignored_addresses) {
  if (manifest_path.empty()) {
    return 0;
  }

  const std::set<uint32_t> merged =
      CollectMergedAddresses(discovered_path, suggestions_path, emit_suggestions, image_base,
                             image_size, ignored_addresses);

  return RepairAndAppendManifestFunctions(manifest_path, section, merged);
}

void WriteBootstrapSuggestionsToml(const std::filesystem::path& path,
                                   const std::set<uint32_t>& addresses) {
  if (addresses.empty()) {
    return;
  }

  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);

  std::ofstream out(path);
  if (!out) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to write suggestions {}", path.string());
    return;
  }

  out << "# Codegen bootstrap suggestions (unresolved call/branch targets)\n[functions]\n";
  for (uint32_t addr : addresses) {
    out << fmt::format("0x{:08X} = {{}}\n", addr);
  }
  REXCODEGEN_INFO("Bootstrap merge: wrote {} suggestion(s) to {}", addresses.size(), path.string());
}

}  // namespace rex::codegen
