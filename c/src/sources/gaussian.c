#include "wave.h"

// Gaussian initial displacement centered on the domain
void init_gaussian(float* u, int nx, int ny, float dx, float dy) {
    float x0 = (nx * dx) / 2.0f;
    float y0 = (ny * dy) / 2.0f;
    float sigma = 0.1f;

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            float x = i * dx;
            float y = j * dy;
            float dist_sq = (x - x0)*(x - x0) + (y - y0)*(y - y0);
            u[IDX(i, j, ny)] = expf(-dist_sq / (sigma * sigma));
        }
    }
}
