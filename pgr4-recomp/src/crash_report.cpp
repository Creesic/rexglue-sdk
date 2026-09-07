// pgr4_recompiled - crash reporter.
//
// A guest or host fault that no SDK handler consumes (memory-watch faults are
// handled before this runs) otherwise ends the process with nothing in the
// log. Log the fault, a symbolized host backtrace (recompiled guest functions
// keep their sub_82XXXXXX names in the PDB), write a minidump beside the log
// and flush, then let the process die as before.

#include "crash_report.h"

#include <rex/platform.h>

#if REX_PLATFORM_WIN32

#include <windows.h>
// dbghelp.h needs windows.h first.
#include <dbghelp.h>

#include <atomic>
#include <cstring>
#include <filesystem>
#include <string>

#include <rex/exception_handler.h>
#include <rex/filesystem.h>
#include <rex/logging.h>

namespace {

std::atomic<bool> g_reporting{false};

std::string Symbolize(DWORD64 address) {
  alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + 256] = {};
  auto* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = 255;

  HMODULE module = nullptr;
  char modulePath[MAX_PATH] = "?";
  if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(address), &module)) {
    GetModuleFileNameA(module, modulePath, MAX_PATH);
  }
  const char* moduleName = std::strrchr(modulePath, '\\');
  moduleName = moduleName != nullptr ? moduleName + 1 : modulePath;

  DWORD64 displacement = 0;
  if (!SymFromAddr(GetCurrentProcess(), address, &displacement, symbol)) {
    return fmt::format("{}+0x{:X}", moduleName,
                       address - reinterpret_cast<DWORD64>(module));
  }
  IMAGEHLP_LINE64 line = {};
  line.SizeOfStruct = sizeof(line);
  DWORD lineDisplacement = 0;
  if (SymGetLineFromAddr64(GetCurrentProcess(), address, &lineDisplacement, &line)) {
    return fmt::format("{}!{}+0x{:X} ({}:{})", moduleName, symbol->Name, displacement,
                       line.FileName, line.LineNumber);
  }
  return fmt::format("{}!{}+0x{:X}", moduleName, symbol->Name, displacement);
}

// The vectored handler runs on the faulting thread's stack, so walking up from
// here passes through the exception dispatcher into the faulting frames.
void LogBacktrace() {
  void* frames[64];
  const USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);
  for (USHORT i = 0; i < count; ++i) {
    REXLOG_ERROR("  #{:02} {}", i, Symbolize(reinterpret_cast<DWORD64>(frames[i])));
  }
}

void WriteMinidump(const char* kind, MINIDUMP_TYPE type) {
  const std::filesystem::path path =
      rex::filesystem::GetExecutableFolder() / "logs" /
      fmt::format("pgr4_recompiled_{}_{}.dmp", kind, GetCurrentProcessId());
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    REXLOG_ERROR("Crash: could not create {}", path.string());
    return;
  }
  const BOOL written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
                                         nullptr, nullptr, nullptr);
  CloseHandle(file);
  REXLOG_ERROR("Crash: minidump {} {}", written ? "written to" : "failed for", path.string());
}

bool OnUnhandledFault(rex::arch::Exception* ex, void* /*data*/) {
  // A fault inside the reporter itself must not recurse.
  if (g_reporting.exchange(true))
    return false;
  const auto pc = ex->pc();
  if (ex->code() == rex::arch::Exception::Code::kAccessViolation) {
    const char* operation =
        ex->access_violation_operation() == rex::arch::Exception::AccessViolationOperation::kWrite
            ? "write"
        : ex->access_violation_operation() == rex::arch::Exception::AccessViolationOperation::kRead
            ? "read"
            : "access";
    REXLOG_ERROR("Crash: {} of 0x{:016X} faulted at 0x{:016X} {} (thread {})", operation,
                 ex->fault_address(), pc, Symbolize(pc), GetCurrentThreadId());
  } else {
    REXLOG_ERROR("Crash: illegal instruction at 0x{:016X} {} (thread {})", pc, Symbolize(pc),
                 GetCurrentThreadId());
  }
  LogBacktrace();
  WriteMinidump("crash", MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory |
                                         MiniDumpWithThreadInfo | MiniDumpWithHandleData));
  rex::FlushLogging();
  return false;  // not handled: the process still terminates, the log has the site
}

}  // namespace

void WriteHangDump(uint32_t stalledSeconds) {
  static std::atomic<bool> s_dumped{false};
  if (s_dumped.exchange(true))
    return;
  REXLOG_ERROR("Hang: no GPU kick for {} s, writing a minidump of all threads", stalledSeconds);
  // Full memory: the guest arena is what needs reading (car-select hang:
  // the vehicle object and guest stack behind a garbage grid position).
  WriteMinidump("hang", MINIDUMP_TYPE(MiniDumpWithFullMemory | MiniDumpWithThreadInfo |
                                        MiniDumpWithHandleData));
  rex::FlushLogging();
}

void InstallCrashReporter() {
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
  SymInitialize(GetCurrentProcess(), nullptr, TRUE);
  // After the runtime's own handlers (memory watch), so only unhandled faults
  // reach the reporter.
  rex::arch::ExceptionHandler::Install(OnUnhandledFault, nullptr);
  REXLOG_INFO("Crash reporter installed (symbolized backtrace + minidump in logs/)");
}

#else

void InstallCrashReporter() {}
void WriteHangDump(uint32_t) {}

#endif
