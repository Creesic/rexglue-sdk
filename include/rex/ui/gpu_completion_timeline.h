#pragma once

#include <algorithm>
#include <cstdint>

#include <rex/assert.h>

namespace rex {
namespace ui {

class GPUCompletionTimeline {
 public:
  GPUCompletionTimeline(const GPUCompletionTimeline&) = delete;
  GPUCompletionTimeline& operator=(const GPUCompletionTimeline&) = delete;
  GPUCompletionTimeline(GPUCompletionTimeline&&) = delete;
  GPUCompletionTimeline& operator=(GPUCompletionTimeline&&) = delete;

  virtual ~GPUCompletionTimeline() = default;

  uint64_t GetUpcomingSubmission() const { return upcoming_submission_; }

  virtual void UpdateCompletedSubmission() = 0;

  uint64_t GetCompletedSubmissionFromLastUpdate() const {
    return completed_submission_;
  }

  uint64_t UpdateAndGetCompletedSubmission() {
    UpdateCompletedSubmission();
    return GetCompletedSubmissionFromLastUpdate();
  }

  bool AwaitSubmissionAndUpdateCompleted(uint64_t awaited_submission) {
    assert_true(awaited_submission < upcoming_submission_);
    if (UpdateAndGetCompletedSubmission() < awaited_submission) {
      AwaitSubmissionImpl(awaited_submission);
    }
    return UpdateAndGetCompletedSubmission() >= awaited_submission;
  }

  bool AwaitMaxSubmissionsPendingAndUpdateCompleted(
      uint64_t max_submissions_pending) {
    assert_not_zero(max_submissions_pending);
    return AwaitSubmissionAndUpdateCompleted(
        GetUpcomingSubmission() -
        std::min(max_submissions_pending, GetUpcomingSubmission()));
  }

  bool AwaitAllSubmissions() {
    return AwaitSubmissionAndUpdateCompleted(GetUpcomingSubmission() - 1);
  }

 protected:
  GPUCompletionTimeline() = default;

  void IncrementUpcomingSubmission() { ++upcoming_submission_; }

  void SetCompletedSubmission(uint64_t new_completed_submission) {
    assert_true(new_completed_submission < upcoming_submission_);
    assert_true(new_completed_submission >= completed_submission_);
    completed_submission_ = new_completed_submission;
  }

  virtual void AwaitSubmissionImpl(uint64_t awaited_submission) = 0;

 private:
  uint64_t upcoming_submission_ = 1;
  uint64_t completed_submission_ = 0;
};

}  // namespace ui
}  // namespace rex
