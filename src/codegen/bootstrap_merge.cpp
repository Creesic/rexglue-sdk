/**
 * @file        codegen/bootstrap_merge.cpp
 * @brief       Merge bootstrap-discovered function stubs into config TOML
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/codegen/bootstrap_merge.h>

#include <fstream>

#include <fmt/format.h>
#include <toml++/toml.hpp>

#include <rex/logging.h>

#include "codegen_logging.h"

namespace rex::codegen {

namespace {

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

std::set<uint32_t> LoadAddressesFromTomlFile(const std::filesystem::path& path) {
  std::set<uint32_t> addresses;
  if (path.empty() || !std::filesystem::exists(path)) {
    return addresses;
  }

  try {
    const toml::table tbl = toml::parse_file(path.string());
    if (const auto* functions = tbl["functions"].as_table()) {
      for (const auto& [key, node] : *functions) {
        if (uint32_t addr = ParseGuestAddressKey(key.str()); addr != 0) {
          addresses.insert(addr);
        }
      }
    }
  } catch (const toml::parse_error& err) {
    REXCODEGEN_WARN("Bootstrap merge: failed to parse {}: {}", path.string(), err.what());
  }

  return addresses;
}

std::set<uint32_t> LoadExistingConfigAddresses(const std::filesystem::path& config_path) {
  return LoadAddressesFromTomlFile(config_path);
}

}  // namespace

std::set<uint32_t> LoadBootstrapFunctionAddresses(const std::filesystem::path& path) {
  return LoadAddressesFromTomlFile(path);
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
  if (addresses.empty() || config_path.empty()) {
    return 0;
  }

  toml::table tbl;
  try {
    if (std::filesystem::exists(config_path)) {
      tbl = toml::parse_file(config_path.string());
    }
  } catch (const toml::parse_error& err) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to parse {}: {}", config_path.string(), err.what());
    return 0;
  }

  if (!tbl.contains("functions") || !tbl["functions"].is_table()) {
    tbl.insert_or_assign("functions", toml::table{});
  }
  auto& functions = *tbl["functions"].as_table();
  std::set<uint32_t> existing;
  for (const auto& [key, node] : functions) {
    if (uint32_t addr = ParseGuestAddressKey(key.str()); addr != 0) {
      existing.insert(addr);
    }
  }

  size_t added = 0;
  for (uint32_t addr : addresses) {
    const std::string key = fmt::format("0x{:08X}", addr);
    if (existing.contains(addr) || functions.contains(key)) {
      continue;
    }
    functions.insert(key, toml::table{});
    ++added;
  }

  if (added == 0) {
    return 0;
  }

  std::ofstream out(config_path);
  if (!out) {
    REXCODEGEN_ERROR("Bootstrap merge: failed to write {}", config_path.string());
    return 0;
  }

  out << tbl;
  REXCODEGEN_INFO("Bootstrap merge: added {} function stub(s) to {}", added, config_path.string());
  return added;
}

size_t MergeBootstrapIntoConfigIncludes(
    const std::filesystem::path& config_dir, const std::vector<std::string>& include_files,
    const std::filesystem::path& discovered_path,
    const std::filesystem::path& suggestions_path, const std::set<uint32_t>& emit_suggestions,
    uint32_t image_base, uint32_t image_size, const std::set<uint32_t>& ignored_addresses) {
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
    REXCODEGEN_WARN("Bootstrap merge: ignored {} discovered/suggested function(s) from config",
                    ignored_count);
  }

  if (merged.empty() || include_files.empty()) {
    return 0;
  }

  size_t total_added = 0;
  for (const auto& include_file : include_files) {
    total_added += MergeBootstrapStubs(config_dir / include_file, merged);
  }
  return total_added;
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
