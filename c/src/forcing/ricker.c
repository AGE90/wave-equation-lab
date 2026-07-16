#include "wave.h"

// Ricker wavelet source (Mexican hat) for time `t` and peak frequency `f`.
float ricker_source(float t, float f) {
    float pi = 3.14159265358979323846f;
    float pi2_f2_t2 = pi * pi * f * f * t * t;
    return (1.0f - 2.0f * pi2_f2_t2) * expf(-pi2_f2_t2);
}
