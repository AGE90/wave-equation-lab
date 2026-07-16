#include "wave.h"

// Simple sinusoidal source
float sine_source(float t, float f) {
    float pi = 3.14159265358979323846f;
    return sinf(2.0f * pi * f * t);
}
