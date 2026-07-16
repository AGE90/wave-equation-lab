#include "wave.h"
#include <string.h>

static void initial_gaussian_impl(float* u_curr, float* u_prev, const Config* cfg) {
    init_gaussian(u_curr, cfg->nx, cfg->ny, cfg->dx, cfg->dy);
    init_gaussian(u_prev, cfg->nx, cfg->ny, cfg->dx, cfg->dy);
}

static void initial_point_impulse_impl(float* u_curr, float* u_prev, const Config* cfg) {
    init_point_impulse(u_curr, cfg->nx, cfg->ny);
    init_point_impulse(u_prev, cfg->nx, cfg->ny);
}

static void initial_zero_impl(float* u_curr, float* u_prev, const Config* cfg) {
    (void)u_curr;
    (void)u_prev;
    (void)cfg;
}

static float forcing_none_impl(float t, const Config* cfg) {
    (void)t;
    (void)cfg;
    return 0.0f;
}

static float forcing_ricker_impl(float t, const Config* cfg) {
    return ricker_source(t, cfg->forcing_f);
}

static float forcing_sine_impl(float t, const Config* cfg) {
    return sine_source(t, cfg->forcing_f);
}

static float forcing_chirp_impl(float t, const Config* cfg) {
    return chirp_source(t, cfg->forcing_f, cfg->forcing_f1, cfg->forcing_t1);
}

typedef struct {
    InitialType type;
    const char* name;
    InitialConditionFn fn;
} InitialRegistryEntry;

typedef struct {
    ForcingType type;
    const char* name;
    ForcingFn fn;
} ForcingRegistryEntry;

static const InitialRegistryEntry INITIAL_REGISTRY[] = {
    {INITIAL_GAUSSIAN, "gaussian", initial_gaussian_impl},
    {INITIAL_POINT_IMPULSE, "point_impulse", initial_point_impulse_impl},
    {INITIAL_ZERO, "zero", initial_zero_impl},
};

static const ForcingRegistryEntry FORCING_REGISTRY[] = {
    {FORCING_NONE, "none", forcing_none_impl},
    {FORCING_RICKER, "ricker", forcing_ricker_impl},
    {FORCING_SINE, "sine", forcing_sine_impl},
    {FORCING_CHIRP, "chirp", forcing_chirp_impl},
};

InitialType parse_initial_type(const char* name, InitialType fallback) {
    if (!name) return fallback;

    if (strcasecmp(name, "impulse") == 0) {
        return INITIAL_POINT_IMPULSE;
    }
    if (strcasecmp(name, "none") == 0) {
        return INITIAL_ZERO;
    }

    size_t n = sizeof(INITIAL_REGISTRY) / sizeof(INITIAL_REGISTRY[0]);
    for (size_t i = 0; i < n; ++i) {
        if (strcasecmp(name, INITIAL_REGISTRY[i].name) == 0) {
            return INITIAL_REGISTRY[i].type;
        }
    }
    return fallback;
}

ForcingType parse_forcing_type(const char* name, ForcingType fallback) {
    if (!name) return fallback;

    size_t n = sizeof(FORCING_REGISTRY) / sizeof(FORCING_REGISTRY[0]);
    for (size_t i = 0; i < n; ++i) {
        if (strcasecmp(name, FORCING_REGISTRY[i].name) == 0) {
            return FORCING_REGISTRY[i].type;
        }
    }
    return fallback;
}

InitialConditionFn resolve_initial_fn(InitialType type) {
    size_t n = sizeof(INITIAL_REGISTRY) / sizeof(INITIAL_REGISTRY[0]);
    for (size_t i = 0; i < n; ++i) {
        if (INITIAL_REGISTRY[i].type == type) {
            return INITIAL_REGISTRY[i].fn;
        }
    }
    return initial_gaussian_impl;
}

ForcingFn resolve_forcing_fn(ForcingType type) {
    size_t n = sizeof(FORCING_REGISTRY) / sizeof(FORCING_REGISTRY[0]);
    for (size_t i = 0; i < n; ++i) {
        if (FORCING_REGISTRY[i].type == type) {
            return FORCING_REGISTRY[i].fn;
        }
    }
    return forcing_none_impl;
}
