/**
 ******************************************************************************
 * ReXGlue FM2 Native Audio Scheduler                                         *
 ******************************************************************************
 */

#include <algorithm>

#include <rex/audio/fm2_native/scheduler.h>

namespace rex::audio::fm2_native {

void HostTimedScheduler::Reset(double initial_credit_bytes) {
  initialized_ = false;
  credit_bytes_ = std::max(0.0, initial_credit_bytes);
}

void HostTimedScheduler::Advance(uint32_t bytes_per_second, uint32_t credit_limit_bytes) {
  if (!initialized_) {
    last_tick_ = clock::now();
    initialized_ = true;
    return;
  }

  const auto now = clock::now();
  const double elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(now - last_tick_).count();
  last_tick_ = now;

  if (elapsed <= 0.0 || bytes_per_second == 0) {
    return;
  }

  credit_bytes_ += elapsed * static_cast<double>(bytes_per_second);
  credit_bytes_ =
      std::clamp(credit_bytes_, 0.0, static_cast<double>(std::max<uint32_t>(credit_limit_bytes, 1)));
}

uint32_t HostTimedScheduler::CurrentBudget(uint32_t max_bytes) const {
  const uint32_t credit_bytes = static_cast<uint32_t>(credit_bytes_ < 0.0 ? 0.0 : credit_bytes_);
  return std::min(credit_bytes, max_bytes);
}

void HostTimedScheduler::Consume(uint32_t bytes) {
  credit_bytes_ -= static_cast<double>(bytes);
  if (credit_bytes_ < 0.0) {
    credit_bytes_ = 0.0;
  }
}

}  // namespace rex::audio::fm2_native

