/**
 ******************************************************************************
 * ReXGlue FM2 Native Audio Scheduler                                         *
 ******************************************************************************
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace rex::audio::fm2_native {

class HostTimedScheduler {
 public:
  void Reset(double initial_credit_bytes = 0.0);
  void Advance(uint32_t bytes_per_second, uint32_t credit_limit_bytes);
  uint32_t CurrentBudget(uint32_t max_bytes) const;
  void Consume(uint32_t bytes);
  double credit_bytes() const { return credit_bytes_; }

 private:
  using clock = std::chrono::steady_clock;
  bool initialized_ = false;
  clock::time_point last_tick_{};
  double credit_bytes_ = 0.0;
};

}  // namespace rex::audio::fm2_native

