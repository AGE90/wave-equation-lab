#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Flat array index formula for row-major 2D array
// Ny is the number of columns (y dimension)
#define IDX(i, j, Ny) ((i) * (Ny) + (j))

// Struct to hold configuration parameters
// New enums to distinguish initial condition and forcing types
typedef enum {
    INITIAL_GAUSSIAN = 0,
    INITIAL_POINT_IMPULSE = 1,
    INITIAL_ZERO = 2
} InitialType;

typedef enum {
    FORCING_NONE = 0,
    FORCING_RICKER = 1,
    FORCING_SINE = 2,
    FORCING_CHIRP = 3
} ForcingType;

typedef enum {
    DAMPING_PROFILE_NONE = 0,
    DAMPING_PROFILE_X_SPLIT = 1,
    DAMPING_PROFILE_CIRCULAR_REGION = 2,
    DAMPING_PROFILE_ABSORBING_BOUNDARY = 3
} DampingProfileType;

typedef struct Config Config;
typedef void (*InitialConditionFn)(float* u_curr, float* u_prev, const Config* cfg);
typedef float (*ForcingFn)(float t, const Config* cfg);

struct Config {
    int nx; // Number of grid points in x direction
    int ny; // Number of grid points in y direction
    int nt; // Number of time steps
    float dt; // Time step size
    float dx; // Grid spacing in x direction
    float dy; // Grid spacing in y direction
    float c_speed;  // Wave speed
    float c_alt;    // Alternative wave speed
    float x_interface; // Interface position in x direction
    int media_type;     // 0: homogeneous, 1: linear split, 2: circular lens
    float lens_radius;  // Radius of the lens
    float lens_x;       // X position of the lens center
    float lens_y;       // Y position of the lens center
    float damping;      // Baseline attenuation coefficient gamma
    DampingProfileType damping_profile_type;
    float damping_alt;  // Alternate attenuation for split/circular profiles
    float damping_x_limit;
    float damping_radius;
    float damping_x;
    float damping_y;
    float damping_width;         // Absorbing boundary width
    float damping_edge;          // Attenuation at the outer boundary
    float damping_power;         // Boundary ramp exponent
    int output_freq; // Frequency of output frames (every N time steps)

    // Fields to explicitly separate initial conditions and forcing
    InitialType initial_type;
    ForcingType forcing_type;
    float forcing_f;   // frequency for ricker/sine
    float forcing_f1;  // end frequency for chirp
    float forcing_t1;  // duration for chirp
    InitialConditionFn initial_fn; // resolved from initial_type by registry
    ForcingFn forcing_fn; // resolved from forcing_type by registry
};

// (Legacy `source_type` removed; use `initial_type` and `forcing`.)

// Function prototypes
Config load_config(const char* filepath);
void write_frame(FILE* bin_fp, float* u, int nx, int ny);
void run_wave_2d(Config cfg, const char* out_filepath);

// Function registry API
InitialType parse_initial_type(const char* name, InitialType fallback);
ForcingType parse_forcing_type(const char* name, ForcingType fallback);
InitialConditionFn resolve_initial_fn(InitialType type);
ForcingFn resolve_forcing_fn(ForcingType type);

// Initial condition functions (initial value problem)
void init_gaussian(float* u, int nx, int ny, float dx, float dy);
void init_point_impulse(float* u, int nx, int ny);

// Forcing functions (time-dependent sources)
float ricker_source(float t, float f);
float sine_source(float t, float f);
float chirp_source(float t, float f0, float f1, float t1);

#endif // WAVE_H
