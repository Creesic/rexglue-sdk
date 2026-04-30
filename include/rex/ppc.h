/**
 * @file        ppc.h
 * @brief       PPC recompilation support -- umbrella header
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <rex/hook.h>
#include <rex/types.h>

#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/ppc/intrinsics.h>
#include <rex/ppc/stack.h>

// Consumer-facing using declarations
using rex::ppc::FindPPCFuncByName;
using rex::ppc::GetPPCFuncRegistry;
using rex::ppc::GuestToHostFunction;
using rex::ppc::HostToGuestFunction;
using rex::ppc::ImportFunction;

// PPC_ macro fallbacks -- these are overridden by the generated init.h.
// Provided here so that plugin-generated init.h files (old format) don't
// cause undefined macro errors in recomp files.
// These use raw volatile access with byte-swap (PPC is big-endian).
#ifndef PPC_FUNC_PROLOGUE
#define PPC_FUNC_PROLOGUE() ((void)0)
#endif
#ifndef PPC_LOAD_U32
#define PPC_LOAD_U32(x) __builtin_bswap32(*(volatile uint32_t*)(base + (uint32_t)(x)))
#endif
#ifndef PPC_STORE_U32
#define PPC_STORE_U32(x, y) (*(volatile uint32_t*)(base + (uint32_t)(x)) = __builtin_bswap32(y))
#endif
#ifndef PPC_LOAD_U16
#define PPC_LOAD_U16(x) __builtin_bswap16(*(volatile uint16_t*)(base + (uint32_t)(x)))
#endif
#ifndef PPC_STORE_U16
#define PPC_STORE_U16(x, y) (*(volatile uint16_t*)(base + (uint32_t)(x)) = __builtin_bswap16(y))
#endif
#ifndef PPC_LOAD_U8
#define PPC_LOAD_U8(x) (*(volatile uint8_t*)(base + (uint32_t)(x)))
#endif
#ifndef PPC_STORE_U8
#define PPC_STORE_U8(x, y) (*(volatile uint8_t*)(base + (uint32_t)(x)) = (y))
#endif
#ifndef PPC_LOAD_U64
#define PPC_LOAD_U64(x) __builtin_bswap64(*(volatile uint64_t*)(base + (uint32_t)(x)))
#endif
#ifndef PPC_STORE_U64
#define PPC_STORE_U64(x, y) (*(volatile uint64_t*)(base + (uint32_t)(x)) = __builtin_bswap64(y))
#endif
