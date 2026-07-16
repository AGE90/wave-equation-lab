#include "wave.h"

// Point impulse initial displacement at the domain center
void init_point_impulse(float* u, int nx, int ny) {
    // assume `u` is zero-initialized (calloc) but set center explicitly
    int center_i = (nx - 1) / 2;
    int center_j = (ny - 1) / 2;
    u[IDX(center_i, center_j, ny)] = 1.0f;
}
