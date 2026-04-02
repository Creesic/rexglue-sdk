/**
 * @file        cmake/mac_compat.h
 * @brief       macOS compatibility shims for Linux-centric code
 *
 * Force-included into every translation unit on Apple platforms via
 *   -include cmake/mac_compat.h
 * in the root CMakeLists.txt.
 *
 * Structural rules:
 *   - Pure macros and typedefs:  always active (safe for C, C++11, C++17+)
 *   - C-compatible inline stubs: inside #ifdef __cplusplus (C++11-safe syntax)
 *   - Polyfills using C++17 features (_v aliases, if constexpr, nested
 *     namespaces, deduced returns): inside #if __cplusplus >= 201703L
 *     so that third-party targets built with -std=c++11 (e.g. glslang) are
 *     completely unaffected.
 *
 * Issues that require source-level edits (handled elsewhere):
 *   - rex::thread::Fiber member declarations  (include/rex/thread/fiber.h)
 *   - exception_handler_posix.cpp mcontext layout
 */
#pragma once
#ifdef __APPLE__

// ── 1. Linux 64-bit file I/O aliases ─────────────────────────────────────────
// Plain typedefs/macros — safe for C, C++11, and C++17+.
// On macOS, off_t is already 64-bit; the Linux *64 variants don't exist.
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#ifndef off64_t
typedef off_t off64_t;
#endif
#ifndef ftruncate64
#  define ftruncate64 ftruncate
#endif
#ifndef mmap64
#  define mmap64      mmap
#endif
#ifndef fseeko64
#  define fseeko64    fseeko
#endif
#ifndef ftello64
#  define ftello64    ftello
#endif
#ifndef fstat64
#  define fstat64     fstat
#endif
// stat64 used as a struct tag ("struct stat64") — map to struct stat.
#ifndef stat64
#  define stat64      stat
#endif

// ── 2. Real-time signal range stub ───────────────────────────────────────────
// macOS has no real-time signals. Map to the two POSIX user signals.
#include <signal.h>
#ifndef SIGRTMIN
#  define SIGRTMIN SIGUSR1
#endif
#ifndef SIGRTMAX
#  define SIGRTMAX SIGUSR2
#endif

// ── 3. IPPROTO_* macro conflicts ─────────────────────────────────────────────
// macOS defines these as plain #defines; Linux defines them as enum values.
// The codebase uses them as scoped enum members (Protocol::IPPROTO_UDP etc.),
// which breaks when a macro named IPPROTO_UDP exists.
#include <netinet/in.h>
#include <sys/socket.h>

#ifdef IPPROTO_UDP
#  undef IPPROTO_UDP
#endif
#ifdef IPPROTO_TCP
#  undef IPPROTO_TCP
#endif
#ifdef IPPROTO_IP
#  undef IPPROTO_IP
#endif
#ifdef IPPROTO_IPV6
#  undef IPPROTO_IPV6
#endif
#ifdef IPPROTO_ICMP
#  undef IPPROTO_ICMP
#endif
#ifdef AF_INET
#  undef AF_INET
#endif
#ifdef AF_INET6
#  undef AF_INET6
#endif
#ifdef AF_UNSPEC
#  undef AF_UNSPEC
#endif
#ifdef SOCK_STREAM
#  undef SOCK_STREAM
#endif
#ifdef SOCK_DGRAM
#  undef SOCK_DGRAM
#endif
#ifdef SOCK_RAW
#  undef SOCK_RAW
#endif

// ── 4. getpagesize ───────────────────────────────────────────────────────────
// _DARWIN_C_SOURCE hides getpagesize(). Use static inline + C cast so this is
// valid in C89/C99/C11 and all C++ standards alike.
#ifndef getpagesize
static inline int getpagesize(void) { return (int)sysconf(_SC_PAGESIZE); }
#endif

// ── 5. CPU affinity stubs — plain macros (C/C++11-safe) ──────────────────────
#include <stdint.h>
typedef struct { uint64_t __bits; } cpu_set_t;
#define CPU_SETSIZE 64u
#define CPU_ZERO(s)    ((s)->__bits = 0)
#define CPU_SET(n, s)  ((s)->__bits |=  (UINT64_C(1) << (n)))
#define CPU_CLR(n, s)  ((s)->__bits &= ~(UINT64_C(1) << (n)))
#define CPU_ISSET(n, s)(!!((s)->__bits &  (UINT64_C(1) << (n))))

// ─────────────────────────────────────────────────────────────────────────────
// C++ only — skipped in C translation units
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __cplusplus

#include <pthread.h>

// ── 5b. CPU affinity inline stubs ────────────────────────────────────────────
// No equivalent of pthread_{get,set}affinity_np on macOS; silently ignored.
// C++11-safe.
inline int pthread_getaffinity_np(pthread_t, size_t, cpu_set_t* s) noexcept {
  s->__bits = ~UINT64_C(0);  // report all CPUs available
  return 0;
}
inline int pthread_setaffinity_np(pthread_t, size_t, const cpu_set_t*) noexcept {
  return 0;  // silently ignored
}

// ─────────────────────────────────────────────────────────────────────────────
// C++17 and later only
// Third-party targets built with -std=c++11 (e.g. glslang) skip this entire
// block. All of the polyfills below use _v aliases, if constexpr, nested
// namespace syntax, or deduced return types that require C++17+.
// ─────────────────────────────────────────────────────────────────────────────
#if __cplusplus >= 201703L

// ── 4. std::move_only_function polyfill ──────────────────────────────────────
// C++23 — not yet shipped in Apple's libc++.
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef __cpp_lib_move_only_function
namespace std {

template <typename Sig>
class move_only_function;

template <typename Ret, typename... Args>
class move_only_function<Ret(Args...)> {
  struct Base {
    virtual ~Base() = default;
    virtual Ret invoke(Args&&...) = 0;
  };
  template <typename F>
  struct Impl final : Base {
    F f;
    explicit Impl(F&& fn) : f(std::move(fn)) {}
    Ret invoke(Args&&... args) override { return f(std::forward<Args>(args)...); }
  };
  unique_ptr<Base> ptr_;

 public:
  move_only_function() noexcept = default;

  // Accept nullptr explicitly so it doesn't fall through to the templated ctor.
  move_only_function(nullptr_t) noexcept : ptr_(nullptr) {}

  template <typename F,
            typename = enable_if_t<!is_same_v<decay_t<F>, move_only_function> &&
                                   !is_same_v<decay_t<F>, nullptr_t>>>
  move_only_function(F&& f) {
    // Don't wrap a null function/member pointer — leave ptr_ null instead.
    if constexpr (is_pointer_v<decay_t<F>> || is_member_pointer_v<decay_t<F>>) {
      if (!f) return;
    }
    ptr_ = make_unique<Impl<decay_t<F>>>(std::forward<decay_t<F>>(f));
  }

  move_only_function(move_only_function&&) = default;
  move_only_function& operator=(move_only_function&&) = default;
  move_only_function(const move_only_function&) = delete;
  move_only_function& operator=(const move_only_function&) = delete;

  explicit operator bool() const noexcept { return ptr_ != nullptr; }
  bool operator==(nullptr_t) const noexcept { return ptr_ == nullptr; }
  bool operator!=(nullptr_t) const noexcept { return ptr_ != nullptr; }

  Ret operator()(Args... args) { return ptr_->invoke(std::forward<Args>(args)...); }
};

}  // namespace std
#endif  // __cpp_lib_move_only_function

// ── 5. std::stop_token / std::stop_source / std::jthread polyfills ───────────
// C++20 cooperative cancellation — not yet in Apple's libc++.
#include <atomic>
#include <thread>

#ifndef __cpp_lib_jthread
namespace std {

class stop_token {
 public:
  bool stop_requested() const noexcept { return flag_ && flag_->load(memory_order_acquire); }
  bool stop_possible()  const noexcept { return flag_ != nullptr; }

  bool operator==(const stop_token& o) const noexcept { return flag_ == o.flag_; }
  bool operator!=(const stop_token& o) const noexcept { return flag_ != o.flag_; }

 private:
  friend class stop_source;
  explicit stop_token(shared_ptr<atomic<bool>> f) : flag_(std::move(f)) {}
  shared_ptr<atomic<bool>> flag_;
};

class stop_source {
 public:
  stop_source() : flag_(make_shared<atomic<bool>>(false)) {}

  stop_token get_token() const { return stop_token{flag_}; }

  bool request_stop() noexcept {
    if (!flag_) return false;
    bool expected = false;
    return flag_->compare_exchange_strong(expected, true, memory_order_acq_rel);
  }

  bool stop_requested() const noexcept { return flag_ && flag_->load(memory_order_acquire); }
  bool stop_possible()  const noexcept { return flag_ != nullptr; }

 private:
  shared_ptr<atomic<bool>> flag_;
};

class jthread {
 public:
  using id = thread::id;

  jthread() = default;

  template <typename F, typename... Args>
  explicit jthread(F&& f, Args&&... args) {
    if constexpr (is_invocable_v<F, stop_token, Args...>) {
      thread_ = thread(std::forward<F>(f), stop_src_.get_token(), std::forward<Args>(args)...);
    } else {
      thread_ = thread(std::forward<F>(f), std::forward<Args>(args)...);
    }
  }

  ~jthread() {
    request_stop();
    if (thread_.joinable()) thread_.join();
  }

  jthread(jthread&&) = default;
  jthread& operator=(jthread&&) = default;
  jthread(const jthread&) = delete;
  jthread& operator=(const jthread&) = delete;

  bool       request_stop()    noexcept { return stop_src_.request_stop(); }
  stop_token get_stop_token()  const    { return stop_src_.get_token(); }
  id         get_id()    const noexcept { return thread_.get_id(); }
  bool       joinable()  const noexcept { return thread_.joinable(); }
  void       join()                     { thread_.join(); }
  void       detach()                   { thread_.detach(); }

 private:
  stop_source stop_src_;
  thread      thread_;
};

}  // namespace std
#endif  // __cpp_lib_jthread

// ── 6. std::chrono::clock_time_conversion primary template ───────────────────
// Apple's libc++ does not provide this C++20 customisation point.
// Must be declared before rex/chrono/chrono.h specialises it.
#include <chrono>

namespace std::chrono {
template <typename DestClock, typename SourceClock>
struct clock_time_conversion {};
}  // namespace std::chrono

// ── 7. std::chrono::clock_cast polyfill ──────────────────────────────────────
// C++20 — not yet in Apple's libc++.
namespace std::chrono {
template <typename DestClock, typename SourceClock, typename Duration>
auto clock_cast(const time_point<SourceClock, Duration>& t) {
  return clock_time_conversion<DestClock, SourceClock>{}(t);
}
}  // namespace std::chrono

// ── 8. std::min mixed-integral overload ──────────────────────────────────────
// disruptorplus calls std::min(size_t, sequence_t).
// On macOS, size_t = unsigned long and sequence_t = unsigned long long —
// distinct types despite both being 64-bit, causing deduction failure.
#include <algorithm>

namespace std {
template <typename A, typename B,
          typename = enable_if_t<!is_same_v<A, B> && is_integral_v<A> &&
                                 is_integral_v<B>>>
constexpr auto min(const A& a, const B& b) noexcept {
  using C = common_type_t<A, B>;
  return static_cast<C>(a) < static_cast<C>(b) ? static_cast<C>(a)
                                                : static_cast<C>(b);
}
template <typename A, typename B,
          typename = enable_if_t<!is_same_v<A, B> && is_integral_v<A> &&
                                 is_integral_v<B>>>
constexpr auto max(const A& a, const B& b) noexcept {
  using C = common_type_t<A, B>;
  return static_cast<C>(a) > static_cast<C>(b) ? static_cast<C>(a)
                                                : static_cast<C>(b);
}
}  // namespace std

#endif  // __cplusplus >= 201703L

#endif  // __cplusplus

#endif  // __APPLE__
