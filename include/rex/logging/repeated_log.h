/**
 * @file        rex/logging/repeated_log.h
 * @brief       Helpers for throttling repeated hot-path log messages
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for more details.
 */

#pragma once

#include <atomic>
#include <cstdint>

namespace rex::log {

class RepeatedLogCounter {
 public:
  static constexpr uint64_t kInitialLogCount = 8;
  static constexpr uint64_t kRepeatInterval = 4096;

  bool ShouldLog(uint64_t* occurrence = nullptr, uint64_t* suppressed = nullptr) noexcept {
    uint64_t current = count_.fetch_add(1, std::memory_order_relaxed) + 1;
    bool should_log = current <= kInitialLogCount || current % kRepeatInterval == 0;
    if (occurrence) {
      *occurrence = current;
    }
    if (suppressed) {
      if (!should_log || current <= kInitialLogCount) {
        *suppressed = 0;
      } else if (current == kRepeatInterval) {
        *suppressed = kRepeatInterval - kInitialLogCount - 1;
      } else {
        *suppressed = kRepeatInterval - 1;
      }
    }
    return should_log;
  }

 private:
  std::atomic<uint64_t> count_{0};
};

}  // namespace rex::log
