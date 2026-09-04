#ifdef __cplusplus
#pragma once
namespace fm2::render {
#endif

// The longest XY edge is the diagonal. These strict comparisons and cyclic
// order match Xenia's rectangle-list primitive expansion, including ties.
inline int RectangleFirstVertex(float edge12, float edge20, float edge01) {
  return edge12 > edge20 && edge12 > edge01 ? 0 : (edge20 > edge01 ? 1 : 2);
}

// Applied independently to clip position, interpolators and clip distance.
inline float RectangleFourthComponent(float first, float second, float third) {
  return (second - first) + third;
}

#ifdef __cplusplus
}  // namespace fm2::render
#endif
