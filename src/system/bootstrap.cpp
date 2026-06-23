/**
 * @file        system/bootstrap.cpp
 * @brief       Bring-up helpers for discovering missing guest function stubs
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/runtime/bootstrap.h>

#include <cctype>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/runtime.h>

REXCVAR_DEFINE_BOOL(bootstrap_unregistered_functions, false, "Bootstrap",
                     "Stub unregistered guest calls and record addresses for codegen");
REXCVAR_DEFINE_STRING(bootstrap_functions_log, "", "Bootstrap",
                      "Append discovered guest function addresses as TOML stubs "
                      "(default: <user_data>/bootstrap_discovered.toml)");

namespace rex::runtime {

namespace {

std::mutex g_bootstrap_mutex;
std::unordered_set<uint32_t> g_recorded_addresses;
std::filesystem::path g_loaded_bootstrap_path;
bool g_bootstrap_cache_loaded = false;

void RewriteBootstrapFunctionsToml(const std::filesystem::path& path,
                                   const std::unordered_set<uint32_t>& addresses,
                                   size_t duplicate_line_count);

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

void LoadBootstrapCacheFromFile(const std::filesystem::path& path) {
  if (g_bootstrap_cache_loaded && g_loaded_bootstrap_path == path) {
    return;
  }

  g_recorded_addresses.clear();
  if (path.empty() || !std::filesystem::exists(path)) {
    g_loaded_bootstrap_path = path;
    g_bootstrap_cache_loaded = true;
    return;
  }

  std::ifstream in(path);
  if (!in) {
    g_loaded_bootstrap_path = path;
    g_bootstrap_cache_loaded = true;
    return;
  }

  size_t address_line_count = 0;
  std::string line;
  while (std::getline(in, line)) {
    uint32_t addr = 0;
    if (TryParseBootstrapAddressLine(line, &addr)) {
      ++address_line_count;
      g_recorded_addresses.insert(addr);
    }
  }

  g_loaded_bootstrap_path = path;
  g_bootstrap_cache_loaded = true;

  if (address_line_count > g_recorded_addresses.size()) {
    RewriteBootstrapFunctionsToml(path, g_recorded_addresses,
                                  address_line_count - g_recorded_addresses.size());
  }
}

std::filesystem::path ResolveBootstrapLogPath() {
  const std::string configured = REXCVAR_GET(bootstrap_functions_log);
  if (!configured.empty()) {
    return std::filesystem::path(configured);
  }

  if (Runtime* runtime = Runtime::instance()) {
    if (!runtime->user_data_root().empty()) {
      return runtime->user_data_root() / "bootstrap_discovered.toml";
    }
  }

  return std::filesystem::current_path() / "bootstrap_discovered.toml";
}

void RewriteBootstrapFunctionsToml(const std::filesystem::path& path,
                                   const std::unordered_set<uint32_t>& addresses,
                                   size_t duplicate_line_count) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);

  auto tmp_path = path;
  tmp_path += ".tmp";
  {
    std::ofstream out(tmp_path, std::ios::binary);
    if (!out) {
      REXLOG_ERROR("Bootstrap: failed to compact log file {}", path.string());
      return;
    }
    out << "# Auto-discovered guest functions (bootstrap mode)\n[functions]\n";
    std::vector<uint32_t> sorted(addresses.begin(), addresses.end());
    std::sort(sorted.begin(), sorted.end());
    for (uint32_t addr : sorted) {
      out << fmt::format("0x{:08X} = {{}}\n", addr);
    }
  }

  std::filesystem::rename(tmp_path, path, ec);
  if (ec) {
    std::filesystem::remove(tmp_path, ec);
    REXLOG_ERROR("Bootstrap: failed to replace compacted log file {}", path.string());
    return;
  }

  REXLOG_INFO("Bootstrap: compacted {} ({} duplicate line(s) removed)", path.string(),
              duplicate_line_count);
}

void AppendBootstrapTomlLine(const std::filesystem::path& path, uint32_t guest_address) {
  std::error_code ec;
  std::filesystem::create_directories(path.parent_path(), ec);

  const bool exists = std::filesystem::exists(path, ec);
  std::ofstream out(path, std::ios::app);
  if (!out) {
    REXLOG_ERROR("Bootstrap: failed to open log file {}", path.string());
    return;
  }

  if (!exists) {
    out << "# Auto-discovered guest functions (bootstrap mode)\n[functions]\n";
  }
  out << fmt::format("0x{:08X} = {{}}\n", guest_address);
  out.flush();
}

}  // namespace

void RecordBootstrapGuestFunction(uint32_t guest_address, const char* source) {
  if (guest_address == 0) {
    return;
  }

  const std::filesystem::path path = ResolveBootstrapLogPath();
  bool is_new = false;
  {
    std::lock_guard lock(g_bootstrap_mutex);
    LoadBootstrapCacheFromFile(path);
    is_new = g_recorded_addresses.insert(guest_address).second;
  }

  if (!is_new) {
    return;
  }

  REXLOG_CRITICAL("BOOTSTRAP_FUNCTION 0x{:08X} ({})", guest_address, source ? source : "unknown");
  AppendBootstrapTomlLine(path, guest_address);
}

void BootstrapOrFatal(PPCContext& ctx, const char* kind, uint32_t site, uint32_t target) {
  if (REXCVAR_GET(bootstrap_unregistered_functions)) {
    (void)ctx;
    RecordBootstrapGuestFunction(target, kind);
    return;
  }

  if (site != 0) {
    REX_FATAL("Unresolved {} from 0x{:08X} to 0x{:08X}", kind ? kind : "call", site, target);
  }
  REX_FATAL("Unresolved {} to 0x{:08X}", kind ? kind : "call", target);
}

}  // namespace rex::runtime
