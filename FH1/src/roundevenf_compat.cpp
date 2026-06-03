#include <cmath>

extern "C" float roundevenf(float v) {
    float rounded = std::roundf(v);
    float diff = rounded - v;
    if (std::fabsf(diff) == 0.5f && (static_cast<int32_t>(rounded) & 1))
        rounded = v - diff;
    return rounded;
}
