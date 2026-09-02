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

#include <filesystem>
#include <fstream>
#include <mutex>
#include <unordered_set>

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
}

}  // namespace

void RecordBootstrapGuestFunction(uint32_t guest_address, const char* source) {
  if (guest_address == 0) {
    return;
  }

  bool is_new = false;
  {
    std::lock_guard lock(g_bootstrap_mutex);
    is_new = g_recorded_addresses.insert(guest_address).second;
  }

  if (!is_new) {
    return;
  }

  REXLOG_CRITICAL("BOOTSTRAP_FUNCTION 0x{:08X} ({})", guest_address, source ? source : "unknown");
  AppendBootstrapTomlLine(ResolveBootstrapLogPath(), guest_address);
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
