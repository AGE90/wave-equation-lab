#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Flat array index formula for row-major 2D array
// Ny is the number of columns (y dimension)
#define IDX(i, j, Ny) ((i) * (Ny) + (j))

// Struct to hold configuration parameters
typedef struct {
    int nx;
    int ny;
    int nt;
    float dt;
    float dx;
    float dy;
    float c_speed;
    float c_alt;
    float x_interface;
    int media_type;     // 0: homogeneous, 1: linear split, 2: circular lens
    float lens_radius;
    float lens_x;
    float lens_y;
    int source_type; // 0: gaussian, 1: ricker wavelet at center
    int output_freq;
} Config;

// Function prototypes
Config load_config(const char* filepath);
void write_frame(FILE* bin_fp, float* u, int nx, int ny);
void run_wave_2d(Config cfg, const char* out_filepath);

#endif // WAVE_H
