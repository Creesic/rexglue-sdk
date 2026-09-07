// pgr4_recompiled - gameplay data fixes.
//
// Lotus Esprit Essex Turbo (car-select hang): the type-4 attachment record in
// Game/Cars/Lotus_EspritEssexTurbo.pak_hrd carries uninitialised exporter
// memory as its second matrix (the only such record across all 123 cars), so
// the attachment's local offset loads as (1.9e37, denormal, denormal). Every
// frame the attachment ground probe (sub_8241BDA8) hands that position to the
// world grid height query (sub_822A53E0), whose cell loop is
// `for (i = lo; i <= hi; ++i)` on 32-bit ints: fctiwz saturates both bounds to
// INT_MAX, the increment wraps, and the loop runs 2^32 times per call.
// A position that saturates the bounds is outside the grid, so answer the
// query's own miss value (-10000) instead of entering the loop.

#include <cstdint>

#include <rex/hook.h>
#include <rex/types.h>

namespace {

// double World_GridHeightQuery(world, pos, radius, yOffset)
//   world+0  grid (0 = no world, the original returns -10000 itself)
//   world+16 grid origin (float4)
//   grid+24  cells per metre
// Bounds per axis: float((pos - origin) +/- radius) * scale, then fctiwz.
REX_EXTERN(__imp__World_GridHeightQuery);

REX_HOOK_RAW(World_GridHeightQuery) {
  const uint32_t world = ctx.r3.u32;
  const uint32_t grid = reinterpret_cast<const rex::be<uint32_t>*>(base + world)->get();
  if (grid != 0) {
    const auto* origin = reinterpret_cast<const rex::be<float>*>(base + world + 16);
    const auto* pos = reinterpret_cast<const rex::be<float>*>(base + ctx.r4.u32);
    const float scale = reinterpret_cast<const rex::be<float>*>(base + grid + 24)->get();
    const float radius = float(ctx.f1.f64);
    for (const int axis : {0, 2}) {
      const float centre = pos[axis].get() - origin[axis].get();
      const float lo = (centre - radius) * scale;
      const float hi = (centre + radius) * scale;
      // fctiwz clamps to INT_MAX from 2^31 up; the loop then wraps around.
      if (lo >= 2147483648.0f || hi >= 2147483648.0f) {
        ctx.f1.f64 = -10000.0;
        return;
      }
    }
  }
  __imp__World_GridHeightQuery(ctx, base);
}

}  // namespace
