// fh1-patcher — standalone test/diagnostic runner for the codegen patcher.
//
// Usage:
//   fh1-patcher <path-to-out-v4>
//
// Calls apply_all() once and exits. Useful for one-shot CI / manual smoke
// testing. The real production integration is via installer_wizard.cpp,
// which calls apply_all() after codegen runs as part of the install flow.
//
// Exit code:
//   0 — all patches applied or already-applied (no failures)
//   1 — at least one patch failed (anchor missing, ambiguous, etc.)
//   2 — usage / IO error

#include <filesystem>
#include <string>

#include <fmt/format.h>

#include "patcher.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fmt::print(stderr, "usage: fh1-patcher <codegen-out-dir>\n");
        return 2;
    }
    std::filesystem::path dir(argv[1]);
    if (!std::filesystem::is_directory(dir)) {
        fmt::print(stderr, "error: {} is not a directory\n", dir.string());
        return 2;
    }
    auto report = fh1::patcher::apply_all(dir);
    fmt::print("fh1-patcher: applied={} already-applied={} failed={}\n",
               report.applied, report.already_applied, report.failed);
    return report.failed > 0 ? 1 : 0;
}
