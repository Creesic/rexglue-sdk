/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

// Disable warnings about unused parameters for kernel functions
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <rex/dbg.h>
#include <rex/kernel/xboxkrnl/private.h>
#include <rex/logging.h>
#include <rex/hook.h>
#include <rex/types.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xexception.h>
#include <rex/system/xthread.h>
#include <rex/system/xtypes.h>

namespace rex::kernel::xboxkrnl {
using namespace rex::system;

PPCContext* GetCurrentGuestContextForDebug() {
  if (auto* thread_state = rex::runtime::ThreadState::Get()) {
    return thread_state->context();
  }

  auto* thread = XThread::GetCurrentThread();
  if (thread && thread->thread_state()) {
    return thread->thread_state()->context();
  }

  return nullptr;
}

uint32_t GetCurrentGuestThreadIdForDebug() {
  if (auto* thread_state = rex::runtime::ThreadState::Get()) {
    return thread_state->thread_id();
  }

  auto* thread = XThread::GetCurrentThread();
  return thread ? thread->thread_id() : 0;
}

void DbgBreakPoint_entry() {
  REXKRNL_ERROR("DbgBreakPoint hit");
  rex::debug::Break();
}

// https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
typedef struct {
  rex::be<uint32_t> type;
  rex::be<uint32_t> name_ptr;
  rex::be<uint32_t> thread_id;
  rex::be<uint32_t> flags;
} X_THREADNAME_INFO;
static_assert_size(X_THREADNAME_INFO, 0x10);

void HandleSetThreadName(ppc_ptr_t<X_EXCEPTION_RECORD> record) {
  // SetThreadName. FFS.
  // https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

  // TODO(benvanik): check record->number_parameters to make sure it's a
  // correct size.
  auto thread_info = reinterpret_cast<X_THREADNAME_INFO*>(&record->exception_information[0]);

  assert_true(thread_info->type == 0x1000);

  if (!thread_info->name_ptr) {
    REXKRNL_DEBUG("SetThreadName called with null name_ptr");
    return;
  }

  // 4D5307D6 (and its demo) has a bug where it ends up passing freed memory for
  // the name, so at the point of SetThreadName it's filled with junk.

  // TODO(gibbed): cvar for thread name encoding for conversion, some games use
  // SJIS and there's no way to automatically know this.
  auto name =
      std::string(REX_KERNEL_MEMORY()->TranslateVirtual<const char*>(thread_info->name_ptr));
  std::replace_if(name.begin(), name.end(), [](auto c) { return c < 32 || c > 127; }, '?');

  object_ref<XThread> thread;
  if (thread_info->thread_id == -1) {
    // Current thread.
    thread = retain_object(XThread::GetCurrentThread());
  } else {
    // Lookup thread by ID.
    thread = REX_KERNEL_STATE()->GetThreadByID(thread_info->thread_id);
  }

  if (thread) {
    REXKRNL_DEBUG("SetThreadName({}, {})", thread->thread_id(), name);
    thread->set_name(name);
  }

  // TODO(benvanik): unwinding required here?
}

typedef struct {
  rex::be<int32_t> mdisp;
  rex::be<int32_t> pdisp;
  rex::be<int32_t> vdisp;
} x_PMD;

typedef struct {
  rex::be<uint32_t> properties;
  rex::be<uint32_t> type_ptr;
  x_PMD this_displacement;
  rex::be<int32_t> size_or_offset;
  rex::be<uint32_t> copy_function_ptr;
} x_s__CatchableType;

typedef struct {
  rex::be<int32_t> number_catchable_types;
  rex::be<uint32_t> catchable_type_ptrs[1];
} x_s__CatchableTypeArray;

typedef struct {
  rex::be<uint32_t> attributes;
  rex::be<uint32_t> unwind_ptr;
  rex::be<uint32_t> forward_compat_ptr;
  rex::be<uint32_t> catchable_type_array_ptr;
} x_s__ThrowInfo;

void HandleCppException(ppc_ptr_t<X_EXCEPTION_RECORD> record) {
  // C++ exception.
  // https://blogs.msdn.com/b/oldnewthing/archive/2010/07/30/10044061.aspx
  // http://www.drdobbs.com/visual-c-exception-handling-instrumentat/184416600
  // http://www.openrce.org/articles/full_view/21

  REXKRNL_ERROR(
      "RtlRaiseException C++ exception: record={:08X} code={:08X} flags={:08X} addr={:08X} "
      "params={} magic={:08X} thrown={:08X} throw_info={:08X}",
      record.guest_address(), static_cast<uint32_t>(record->code),
      static_cast<uint32_t>(record->exception_flags),
      static_cast<uint32_t>(record->exception_address),
      static_cast<uint32_t>(record->number_parameters),
      static_cast<uint32_t>(record->exception_information[0]),
      static_cast<uint32_t>(record->exception_information[1]),
      static_cast<uint32_t>(record->exception_information[2]));

  assert_true(record->number_parameters == 3);
  assert_true(record->exception_information[0] == 0x19930520);

  auto thrown_ptr = record->exception_information[1];
  auto thrown = REX_KERNEL_MEMORY()->TranslateVirtual(thrown_ptr);
  auto vftable_ptr = *reinterpret_cast<rex::be<uint32_t>*>(thrown);

  auto throw_info_ptr = record->exception_information[2];
  auto throw_info = REX_KERNEL_MEMORY()->TranslateVirtual<x_s__ThrowInfo*>(throw_info_ptr);
  auto catchable_types = REX_KERNEL_MEMORY()->TranslateVirtual<x_s__CatchableTypeArray*>(
      throw_info->catchable_type_array_ptr);

  REXKRNL_ERROR(
      "RtlRaiseException C++ details: thrown_vftable={:08X} throw_attrs={:08X} unwind={:08X} "
      "forward={:08X} catchable_array={:08X} catchable_count={}",
      static_cast<uint32_t>(vftable_ptr), static_cast<uint32_t>(throw_info->attributes),
      static_cast<uint32_t>(throw_info->unwind_ptr),
      static_cast<uint32_t>(throw_info->forward_compat_ptr),
      static_cast<uint32_t>(throw_info->catchable_type_array_ptr),
      catchable_types ? static_cast<int32_t>(catchable_types->number_catchable_types) : -1);

  rex::debug::Break();
}

void RtlRaiseException_entry(ppc_ptr_t<X_EXCEPTION_RECORD> record) {
  if (!record) {
    REXKRNL_ERROR("RtlRaiseException called with null record");
    rex::debug::Break();
    return;
  }

  REXKRNL_ERROR(
      "RtlRaiseException: record={:08X} code={:08X} flags={:08X} addr={:08X} params={} "
      "info0={:08X} info1={:08X} info2={:08X} info3={:08X}",
      record.guest_address(), static_cast<uint32_t>(record->code),
      static_cast<uint32_t>(record->exception_flags),
      static_cast<uint32_t>(record->exception_address),
      static_cast<uint32_t>(record->number_parameters),
      static_cast<uint32_t>(record->exception_information[0]),
      static_cast<uint32_t>(record->exception_information[1]),
      static_cast<uint32_t>(record->exception_information[2]),
      static_cast<uint32_t>(record->exception_information[3]));

  switch (record->code) {
    case 0x406D1388: {
      HandleSetThreadName(record);
      return;
    }
    case 0xE06D7363: {
      HandleCppException(record);
      return;
    }
  }

  // TODO(benvanik): unwinding.
  // This is going to suck.
  REXKRNL_ERROR("RtlRaiseException unhandled code={:08X}",
                static_cast<uint32_t>(record->code));
  rex::debug::Break();
}

void KeBugCheckEx_entry(u32 code, u32 param1, u32 param2, u32 param3, u32 param4) {
  auto* ctx = GetCurrentGuestContextForDebug();
  if (ctx) {
    REXKRNL_ERROR(
        "*** STOP: 0x{:08X} (0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}) tid={} lr={:08X} "
        "r1={:08X} r3={:08X} r4={:08X} r5={:08X} r6={:08X} r7={:08X}",
        code, param1, param2, param3, param4, GetCurrentGuestThreadIdForDebug(),
        static_cast<uint32_t>(ctx->lr), static_cast<uint32_t>(ctx->r1.u64),
        static_cast<uint32_t>(ctx->r3.u64), static_cast<uint32_t>(ctx->r4.u64),
        static_cast<uint32_t>(ctx->r5.u64), static_cast<uint32_t>(ctx->r6.u64),
        static_cast<uint32_t>(ctx->r7.u64));
  } else {
    REXKRNL_ERROR(
        "*** STOP: 0x{:08X} (0x{:08X}, 0x{:08X}, 0x{:08X}, 0x{:08X}) no-guest-context",
        code, param1, param2, param3, param4);
  }
  fflush(stdout);
  rex::debug::Break();
  assert_always();
}

void KeBugCheck_entry(u32 code) {
  KeBugCheckEx_entry(code, 0, 0, 0, 0);
}

}  // namespace rex::kernel::xboxkrnl

REX_EXPORT(__imp__DbgBreakPoint, rex::kernel::xboxkrnl::DbgBreakPoint_entry)
REX_EXPORT(__imp__RtlRaiseException, rex::kernel::xboxkrnl::RtlRaiseException_entry)
REX_EXPORT(__imp__KeBugCheckEx, rex::kernel::xboxkrnl::KeBugCheckEx_entry)
REX_EXPORT(__imp__KeBugCheck, rex::kernel::xboxkrnl::KeBugCheck_entry)

REX_EXPORT_STUB(__imp__DbgBreakPointWithStatus);
REX_EXPORT_STUB(__imp__DbgPrompt);
REX_EXPORT_STUB(__imp__DbgLoadImageSymbols);
REX_EXPORT_STUB(__imp__DbgUnLoadImageSymbols);
REX_EXPORT_STUB(__imp__DmPrintData);

REX_EXPORT_STUB(__imp__DumpGetRawDumpInfo);
REX_EXPORT_STUB(__imp__DumpRegisterDedicatedDataBlock);
REX_EXPORT_STUB(__imp__DumpSetCollectionFacility);
REX_EXPORT_STUB(__imp__DumpUpdateDumpSettings);
REX_EXPORT_STUB(__imp__DumpWriteDump);
REX_EXPORT_STUB(__imp__DumpXitThread);
REX_EXPORT_STUB(__imp__RtlAssert);
REX_EXPORT_STUB(__imp__RtlRaiseStatus);
REX_EXPORT_STUB(__imp__RtlRip);
