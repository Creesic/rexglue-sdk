/**
 * @file        ppc/legacy_macros.h
 * @brief       Compatibility macros for older rexglue generated PPC code
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <csetjmp>
#include <cstdlib>
#include <string_view>
#include <unordered_map>

#include <rex/chrono/clock.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include <rex/ppc/intrinsics.h>
#include <rex/system/mmio_handler.h>
#include <rex/thread/mutex.h>

namespace rex {
using ppc::simde_mm_adds_epu32;
using ppc::simde_mm_avg_epi16;
using ppc::simde_mm_avg_epi8;
using ppc::simde_mm_cvtepu32_ps_;
using ppc::simde_mm_perm_epi8_;
using ppc::simde_mm_sllv_epi8;
using ppc::simde_mm_vctsxs;
using ppc::simde_mm_vctuxs;
using ppc::simde_mm_vsl;
using ppc::simde_mm_vslo;
using ppc::simde_mm_vsr;
using ppc::simde_mm_vsro;
}  // namespace rex

#ifndef PPC_THUNK_RESERVE_SIZE
#define PPC_THUNK_RESERVE_SIZE 0x10000ull
#endif

#ifndef PPC_EXTERN_FUNC
#define PPC_EXTERN_FUNC(name) REX_EXTERN(name)
#endif

#ifndef PPC_EXTERN_IMPORT
#define PPC_EXTERN_IMPORT(name) \
  REX_EXTERN(name);             \
  REX_EXTERN(__imp__##name)
#endif

#ifndef PPC_FUNC_IMPL
#define PPC_FUNC_IMPL(name) REX_EXTERN(name)
#endif

#ifndef PPC_WEAK_FUNC
#define PPC_WEAK_FUNC(name) REX_WEAK_FUNC(name)
#endif

#ifndef PPC_FUNC_PROLOGUE
#if defined(REXGLUE_PROFILE_GUEST_FUNCTIONS) && defined(REXGLUE_ENABLE_PROFILING)
#include <tracy/Tracy.hpp>
#if defined(__clang__)
#define PPC_FUNC_PROLOGUE()                     \
  __builtin_assume(((size_t)base & 0x1F) == 0); \
  ZoneNamedN(___tracy_guest_zone, __func__, true)
#elif defined(__GNUC__)
#define PPC_FUNC_PROLOGUE()         \
  do {                              \
    if (((size_t)base & 0x1F) != 0) \
      __builtin_unreachable();      \
  } while (0);                      \
  ZoneNamedN(___tracy_guest_zone, __func__, true)
#else
#define PPC_FUNC_PROLOGUE() ZoneNamedN(___tracy_guest_zone, __func__, true)
#endif
#else
#if defined(__clang__)
#define PPC_FUNC_PROLOGUE() __builtin_assume(((size_t)base & 0x1F) == 0)
#elif defined(__GNUC__)
#define PPC_FUNC_PROLOGUE()         \
  do {                              \
    if (((size_t)base & 0x1F) != 0) \
      __builtin_unreachable();      \
  } while (0)
#else
#define PPC_FUNC_PROLOGUE() ((void)0)
#endif
#endif
#endif

#ifndef PPC_PHYS_HOST_OFFSET
#if REX_PLATFORM_WIN32
#define PPC_PHYS_HOST_OFFSET(addr) (((u32)(addr) >= 0xE0000000u) ? 0x1000u : 0u)
#else
#define PPC_PHYS_HOST_OFFSET(addr) 0u
#endif
#endif

#ifndef PPC_RAW_ADDR
#define PPC_RAW_ADDR(x) (base + (u32)(x) + PPC_PHYS_HOST_OFFSET(x))
#endif

#ifndef PPC_LOAD_U8
#define PPC_LOAD_U8(x) (*(volatile u8*)PPC_RAW_ADDR(x))
#endif

#ifndef PPC_LOAD_U16
#define PPC_LOAD_U16(x) __builtin_bswap16(*(volatile u16*)PPC_RAW_ADDR(x))
#endif

#ifndef PPC_LOAD_U32
#define PPC_LOAD_U32(x) __builtin_bswap32(*(volatile u32*)PPC_RAW_ADDR(x))
#endif

#ifndef PPC_LOAD_U64
#define PPC_LOAD_U64(x) __builtin_bswap64(*(volatile u64*)PPC_RAW_ADDR(x))
#endif

#ifndef PPC_LOAD_STRING
#define PPC_LOAD_STRING(x, len) std::string_view(reinterpret_cast<const char*>(PPC_RAW_ADDR(x)), (len))
#endif

#ifndef PPC_STORE_U8
#define PPC_STORE_U8(x, y) (*(volatile u8*)PPC_RAW_ADDR(x) = (y))
#endif

#ifndef PPC_STORE_U16
#define PPC_STORE_U16(x, y) (*(volatile u16*)PPC_RAW_ADDR(x) = __builtin_bswap16(y))
#endif

#ifndef PPC_STORE_U32
#define PPC_STORE_U32(x, y) (*(volatile u32*)PPC_RAW_ADDR(x) = __builtin_bswap32(y))
#endif

#ifndef PPC_STORE_U64
#define PPC_STORE_U64(x, y) (*(volatile u64*)PPC_RAW_ADDR(x) = __builtin_bswap64(y))
#endif

#ifndef PPC_MEMORY_SIZE
#define PPC_MEMORY_SIZE 0x100000000ull
#endif

#ifndef PPC_IS_MMIO_ADDR
#define PPC_IS_MMIO_ADDR(addr) ((addr) >= 0x7F000000u && (addr) < 0x80000000u)
#endif

#ifndef PPC_MM_STORE_U8
#define PPC_MM_STORE_U8(addr, val)                                                         \
  do {                                                                                     \
    u32 _ppc_mmio_addr = (addr);                                                          \
    if (PPC_IS_MMIO_ADDR(_ppc_mmio_addr)) {                                                \
      rex::runtime::MMIOHandler::global_handler()->CheckStore(_ppc_mmio_addr,              \
                                                              static_cast<u32>(val));      \
    } else {                                                                               \
      *(volatile u8*)PPC_RAW_ADDR(_ppc_mmio_addr) = (val);                                 \
    }                                                                                      \
  } while (0)
#endif

#ifndef PPC_MM_STORE_U16
#define PPC_MM_STORE_U16(addr, val)                                                        \
  do {                                                                                     \
    u32 _ppc_mmio_addr = (addr);                                                          \
    if (PPC_IS_MMIO_ADDR(_ppc_mmio_addr)) {                                                \
      rex::runtime::MMIOHandler::global_handler()->CheckStore(_ppc_mmio_addr,              \
                                                              static_cast<u32>(val));      \
    } else {                                                                               \
      *(volatile u16*)PPC_RAW_ADDR(_ppc_mmio_addr) = __builtin_bswap16(val);               \
    }                                                                                      \
  } while (0)
#endif

#ifndef PPC_MM_STORE_U32
#define PPC_MM_STORE_U32(addr, val)                                                        \
  do {                                                                                     \
    u32 _ppc_mmio_addr = (addr);                                                          \
    if (PPC_IS_MMIO_ADDR(_ppc_mmio_addr)) {                                                \
      rex::runtime::MMIOHandler::global_handler()->CheckStore(_ppc_mmio_addr,              \
                                                              static_cast<u32>(val));      \
    } else {                                                                               \
      *(volatile u32*)PPC_RAW_ADDR(_ppc_mmio_addr) = __builtin_bswap32(val);               \
    }                                                                                      \
  } while (0)
#endif

#ifndef PPC_MM_STORE_U64
#define PPC_MM_STORE_U64(addr, val)                                                               \
  do {                                                                                            \
    u32 _ppc_mmio_addr = (addr);                                                                 \
    if (PPC_IS_MMIO_ADDR(_ppc_mmio_addr)) {                                                       \
      u64 _ppc_mmio_val = static_cast<u64>(val);                                                  \
      rex::runtime::MMIOHandler::global_handler()->CheckStore(_ppc_mmio_addr,                     \
                                                              static_cast<u32>(_ppc_mmio_val >> 32)); \
      rex::runtime::MMIOHandler::global_handler()->CheckStore(_ppc_mmio_addr + 4,                 \
                                                              static_cast<u32>(_ppc_mmio_val));   \
    } else {                                                                                      \
      *(volatile u64*)PPC_RAW_ADDR(_ppc_mmio_addr) = __builtin_bswap64(val);                      \
    }                                                                                             \
  } while (0)
#endif

#ifndef PPC_MM_LOAD_U8
#define PPC_MM_LOAD_U8(addr)                                                   \
  (PPC_IS_MMIO_ADDR(addr) ? ({                                                 \
    u32 _ppc_mmio_val;                                                         \
    rex::runtime::MMIOHandler::global_handler()->CheckLoad(addr, &_ppc_mmio_val); \
    static_cast<u8>(_ppc_mmio_val);                                            \
  })                                                                           \
                          : *(volatile u8*)PPC_RAW_ADDR(addr))
#endif

#ifndef PPC_MM_LOAD_U16
#define PPC_MM_LOAD_U16(addr)                                                         \
  (PPC_IS_MMIO_ADDR(addr)                                                             \
       ? ({                                                                           \
           u32 _ppc_mmio_val;                                                         \
           rex::runtime::MMIOHandler::global_handler()->CheckLoad(addr, &_ppc_mmio_val); \
           static_cast<u16>(_ppc_mmio_val);                                           \
         })                                                                           \
       : __builtin_bswap16(*(volatile u16*)PPC_RAW_ADDR(addr)))
#endif

#ifndef PPC_MM_LOAD_U32
#define PPC_MM_LOAD_U32(addr)                                                         \
  (PPC_IS_MMIO_ADDR(addr)                                                             \
       ? ({                                                                           \
           u32 _ppc_mmio_val;                                                         \
           rex::runtime::MMIOHandler::global_handler()->CheckLoad(addr, &_ppc_mmio_val); \
           _ppc_mmio_val;                                                             \
         })                                                                           \
       : __builtin_bswap32(*(volatile u32*)PPC_RAW_ADDR(addr)))
#endif

#ifndef PPC_MM_LOAD_U64
#define PPC_MM_LOAD_U64(addr)                                                              \
  (PPC_IS_MMIO_ADDR(addr)                                                                  \
       ? ({                                                                                \
           u32 _ppc_mmio_hi, _ppc_mmio_lo;                                                 \
           rex::runtime::MMIOHandler::global_handler()->CheckLoad(addr, &_ppc_mmio_hi);    \
           rex::runtime::MMIOHandler::global_handler()->CheckLoad((addr) + 4, &_ppc_mmio_lo); \
           (static_cast<u64>(_ppc_mmio_hi) << 32) | _ppc_mmio_lo;                          \
         })                                                                                \
       : __builtin_bswap64(*(volatile u64*)PPC_RAW_ADDR(addr)))
#endif

#ifndef PPC_CALL_FUNC
#define PPC_CALL_FUNC(x) x(ctx, base)
#endif

#ifndef PPC_PROFILE_INDIRECT_DISPATCH
#ifdef REXGLUE_ENABLE_PROFILING
#define PPC_PROFILE_INDIRECT_DISPATCH() PROFILE_FUNCTION_DISPATCHED()
#else
#define PPC_PROFILE_INDIRECT_DISPATCH()
#endif
#endif

#ifndef PPC_LOOKUP_FUNC
#define PPC_LOOKUP_FUNC(x, y) \
  (*(PPCFunc**)(x + PPC_IMAGE_BASE + PPC_IMAGE_SIZE + (u64(u32(y) - PPC_CODE_BASE) * 2)))
#endif

#ifndef PPC_CALL_INDIRECT_FUNC
#define PPC_CALL_INDIRECT_FUNC(x)                                                          \
  do {                                                                                     \
    PPC_PROFILE_INDIRECT_DISPATCH();                                                       \
    uint32_t ppc_indirect_target_ = (uint32_t)(x);                                         \
    PPCFunc* ppc_indirect_func_;                                                           \
    if ((uint32_t)(ppc_indirect_target_ - PPC_CODE_BASE) <                                 \
        PPC_CODE_SIZE + PPC_THUNK_RESERVE_SIZE) [[likely]] {                               \
      ppc_indirect_func_ = PPC_LOOKUP_FUNC(base, ppc_indirect_target_);                    \
    } else {                                                                               \
      ppc_indirect_func_ = nullptr;                                                        \
    }                                                                                      \
    if (!ppc_indirect_func_) [[unlikely]] {                                                \
      ctx.last_indirect_target = ppc_indirect_target_;                                     \
      ppc_indirect_func_ = rex::runtime::ResolveIndirectFunction(ppc_indirect_target_);    \
    }                                                                                      \
    ppc_indirect_func_(ctx, base);                                                         \
  } while (0)
#endif

#ifndef PPC_QUERY_TIMEBASE
#define PPC_QUERY_TIMEBASE() rex::chrono::Clock::QueryGuestTickCount()
#endif

#ifndef REX_CONFIG_H_INCLUDED
// Current generated init headers define these helpers themselves.
inline void ppc_trap(PPCContext& ctx, u8* base, u16 trap_type) {
  switch (trap_type) {
    case 20:
    case 26: {
      auto str = PPC_LOAD_STRING(ctx.r3.u32, ctx.r4.u16);
      REXCPU_DEBUG("(service trap) {}", str);
      break;
    }
    case 0:
    case 22:
      REXCPU_WARN("tw/td trap hit (type {})", trap_type);
      break;
    case 25:
      break;
    default:
      REXCPU_WARN("Unknown trap type {}", trap_type);
      break;
  }
}

struct PPCLegacyJmpBuf {
  jmp_buf buf;
};

inline std::unordered_map<u32, PPCLegacyJmpBuf>& get_legacy_jmp_buf_map() {
  static thread_local std::unordered_map<u32, PPCLegacyJmpBuf> map;
  return map;
}

#ifndef ppc_setjmp
#define ppc_setjmp(guest_buf_addr) (setjmp(::get_legacy_jmp_buf_map()[(guest_buf_addr)].buf))
#endif

[[noreturn]] inline void ppc_longjmp(u32 guest_buf_addr, int val) {
  auto& map = get_legacy_jmp_buf_map();
  auto it = map.find(guest_buf_addr);
  if (it != map.end()) {
    longjmp(it->second.buf, val);
  }
  std::abort();
}

namespace rex {
[[noreturn]] inline void ppc_longjmp(u32 guest_buf_addr, int val) {
  ::ppc_longjmp(guest_buf_addr, val);
}
}  // namespace rex
#endif

inline std::atomic<i32>& ppc_legacy_global_lock_count_() {
  static std::atomic<i32> count{0};
  return count;
}

#ifndef PPC_CHECK_GLOBAL_LOCK
#define PPC_CHECK_GLOBAL_LOCK()                                        \
  ([&]() -> u64 {                                                      \
    auto lock_ = rex::thread::global_critical_region::AcquireDirect(); \
    return ppc_legacy_global_lock_count_().load() ? 0 : 0x8000;        \
  }())
#endif

#ifndef PPC_ENTER_GLOBAL_LOCK
#define PPC_ENTER_GLOBAL_LOCK()                          \
  do {                                                   \
    rex::thread::global_critical_region::mutex().lock(); \
    ppc_legacy_global_lock_count_().fetch_add(1);        \
  } while (0)
#endif

#ifndef PPC_LEAVE_GLOBAL_LOCK
#define PPC_LEAVE_GLOBAL_LOCK()                                                           \
  do {                                                                                    \
    auto old_count_ = ppc_legacy_global_lock_count_().fetch_sub(1);                       \
    assert(old_count_ >= 1 && "LeaveGlobalLock called without matching EnterGlobalLock"); \
    rex::thread::global_critical_region::mutex().unlock();                                \
  } while (0)
#endif
