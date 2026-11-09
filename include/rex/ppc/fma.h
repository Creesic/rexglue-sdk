/**
 * @file        fma.h
 * @brief       Fused multiply-add for recompiled fmadd/fmsub/fnmadd/fnmsub
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <cmath>

#if defined(_M_X64) || defined(__x86_64__)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace rex::ppc {

// PowerPC fmadd rounds once. std::fma does too, but without FMA enabled at
// compile time it becomes a call into the C runtime, whose software fma is
// very slow (the UCRT's took most of a game thread). So on x86-64 the host
// FMA unit is used when the CPU has one (checked once at startup), and older
// CPUs get an exact two-product sum: a few plain operations, the same
// result as a fused multiply-add for all ordinary operands.

#if defined(__FMA__) || defined(__aarch64__) || defined(_M_ARM64) || !(defined(_M_X64) || defined(__x86_64__))

inline double FusedMultiplyAdd(double a, double b, double c) {
  return std::fma(a, b, c);
}

#else

#if defined(__clang__) || defined(__GNUC__)
__attribute__((target("xsave")))
#endif
inline bool DetectHostFma() {
  unsigned int r[4] = {};
#if defined(_MSC_VER)
  int regs[4];
  __cpuid(regs, 1);
  for (int i = 0; i < 4; ++i) r[i] = unsigned(regs[i]);
#else
  __get_cpuid(1, &r[0], &r[1], &r[2], &r[3]);
#endif
  const bool fma = (r[2] >> 12) & 1;
  const bool osxsave = (r[2] >> 27) & 1;
  const bool avx = (r[2] >> 28) & 1;
  if (!fma || !osxsave || !avx) {
    return false;
  }
  // The OS must save the YMM state for VEX-encoded instructions.
  return (_xgetbv(0) & 6) == 6;
}

inline const bool kHostHasFma = DetectHostFma();

#if defined(__clang__) || defined(__GNUC__)
__attribute__((target("fma"))) inline double HardwareFusedMultiplyAdd(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}
#else
inline double HardwareFusedMultiplyAdd(double a, double b, double c) {
  return std::fma(a, b, c);
}
#endif

// a * b + c with one rounding, without an FMA unit: a * b split exactly into
// p + e (Veltkamp/Dekker), p + c into s + t (Knuth two-sum), then
// s + (t + e). Operands whose product or error term could overflow or fall
// into the subnormal range (where flush-to-zero would change them) take
// std::fma instead.
inline double SoftwareFusedMultiplyAdd(double a, double b, double c) {
  const double p = a * b;
  const double ap = std::fabs(p);
  if (!(ap >= 0x1p-900 && ap <= 0x1p990) || !(std::fabs(a) <= 0x1p995) || !(std::fabs(b) <= 0x1p995) ||
      !(std::fabs(c) <= 0x1p1000)) {
    if (p == 0.0 && std::isfinite(c)) {
      return p + c;
    }
    return std::fma(a, b, c);
  }
  constexpr double kSplit = 134217729.0;  // 2^27 + 1
  const double ca = kSplit * a, a_hi = ca - (ca - a), a_lo = a - a_hi;
  const double cb = kSplit * b, b_hi = cb - (cb - b), b_lo = b - b_hi;
  const double e = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
  const double s = p + c;
  const double bb = s - p;
  const double t = (p - (s - bb)) + (c - bb);
  return s + (t + e);
}

inline double FusedMultiplyAdd(double a, double b, double c) {
  return kHostHasFma ? HardwareFusedMultiplyAdd(a, b, c) : SoftwareFusedMultiplyAdd(a, b, c);
}

#endif

}  // namespace rex::ppc
