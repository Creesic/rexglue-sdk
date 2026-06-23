/**
 * @file        rex/codegen/bootstrap_merge.h
 * @brief       Merge bootstrap-discovered function stubs into config/manifest TOML
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <rex/codegen/config.h>

namespace rex::codegen {

/// Load guest function entry addresses from a TOML file's [functions] table.
std::set<uint32_t> LoadBootstrapFunctionAddresses(const std::filesystem::path& path);

/// Load `[section].functions` from a manifest TOML (e.g. section = "entrypoint").
std::set<uint32_t> LoadManifestFunctions(const std::filesystem::path& manifest_path,
                                           std::string_view section);

/// Drop addresses outside [image_base, image_base + image_size).
std::set<uint32_t> FilterBootstrapAddressesInImage(const std::set<uint32_t>& addresses,
                                                   uint32_t image_base, uint32_t image_size);

/// Remove [functions] entries outside the module image from an in-memory config map.
void PruneFunctionsOutsideImage(std::unordered_map<uint32_t, FunctionConfig>& functions,
                                uint32_t image_base, uint32_t image_size);

/**
 * Merge stub entries into configPath's [functions] table.
 * @return Number of addresses newly added to configPath.
 */
size_t MergeBootstrapStubs(const std::filesystem::path& config_path,
                           const std::set<uint32_t>& addresses);

/**
 * Merge addresses from optional bootstrap TOML files and an in-memory set into
 * each config include file (typically *_config.toml).
 */
size_t MergeBootstrapIntoConfigIncludes(
    const std::filesystem::path& config_dir, const std::vector<std::string>& include_files,
    const std::filesystem::path& discovered_path,
    const std::filesystem::path& suggestions_path, const std::set<uint32_t>& emit_suggestions,
    uint32_t image_base = 0, uint32_t image_size = 0,
    const std::set<uint32_t>& ignored_addresses = {});

/**
 * Merge bootstrap discoveries into `[section].functions` of a manifest TOML.
 * @return Number of addresses newly added to the manifest.
 */
size_t MergeBootstrapIntoManifest(const std::filesystem::path& manifest_path,
                                  std::string_view section,
                                  const std::filesystem::path& discovered_path,
                                  const std::filesystem::path& suggestions_path,
                                  const std::set<uint32_t>& emit_suggestions,
                                  uint32_t image_base = 0, uint32_t image_size = 0,
                                  const std::set<uint32_t>& ignored_addresses = {});

/// Write bootstrap codegen suggestions for the next merge pass.
void WriteBootstrapSuggestionsToml(const std::filesystem::path& path,
                                   const std::set<uint32_t>& addresses);

}  // namespace rex::codegen
