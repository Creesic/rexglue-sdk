/**
 * @file        runtime/bootstrap.h
 * @brief       Bring-up helpers for discovering missing guest function stubs
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <cstdint>

#include <rex/cvar.h>
#include <rex/ppc/context.h>

REXCVAR_DECLARE(bool, bootstrap_unregistered_functions);
REXCVAR_DECLARE(std::string, bootstrap_functions_log);

namespace rex::runtime {

/// Record a guest address discovered during bootstrap bring-up.
void RecordBootstrapGuestFunction(uint32_t guest_address, const char* source);

/**
 * Log/record the target and return when bootstrap mode is enabled; otherwise fatal.
 * Used by generated code for statically unresolved call/branch sites.
 */
void BootstrapOrFatal(PPCContext& ctx, const char* kind, uint32_t site, uint32_t target);

}  // namespace rex::runtime
