#include "wave.h"

// Linear chirp from f0 to f1 over duration t1. Returns instantaneous amplitude at time t.
float chirp_source(float t, float f0, float f1, float t1) {
    float pi = 3.14159265358979323846f;
    if (t1 <= 0.0f) return sinf(2.0f * pi * f0 * t);
    float k = (f1 - f0) / t1; // rate of frequency change
    // instantaneous phase = 2*pi*(f0*t + 0.5*k*t^2)
    float phase = 2.0f * pi * (f0 * t + 0.5f * k * t * t);
    return sinf(phase);
}
