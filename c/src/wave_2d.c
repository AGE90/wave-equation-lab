#include "wave.h"

/* Initial-condition and forcing implementations are in separate modules under:
    - c/src/sources/ (initial condition modules)
    - c/src/forcing/ (time-dependent source modules)
    See `c/include/wave.h` for prototypes. */

static float damping_at(const Config* cfg, float x, float y) {
    float gamma = cfg->damping;

    if (cfg->damping_profile_type == DAMPING_PROFILE_X_SPLIT) {
        if (x < cfg->damping_x_limit) gamma = cfg->damping_alt;
    } else if (cfg->damping_profile_type == DAMPING_PROFILE_CIRCULAR_REGION) {
        float dx = x - cfg->damping_x;
        float dy = y - cfg->damping_y;
        if (dx * dx + dy * dy < cfg->damping_radius * cfg->damping_radius) gamma = cfg->damping_alt;
    } else if (cfg->damping_profile_type == DAMPING_PROFILE_ABSORBING_BOUNDARY) {
        float domain_x = (cfg->nx - 1) * cfg->dx;
        float domain_y = (cfg->ny - 1) * cfg->dy;
        float distance_to_edge = fminf(fminf(x, domain_x - x), fminf(y, domain_y - y));
        if (distance_to_edge < cfg->damping_width) {
            float penetration = 1.0f - distance_to_edge / cfg->damping_width;
            gamma = cfg->damping + (cfg->damping_edge - cfg->damping) * powf(penetration, cfg->damping_power);
        }
    }

    return gamma;
}

void run_wave_2d(Config cfg, const char* out_filepath) {
    // Arrays for history
    int N = cfg.nx * cfg.ny;
    float* u_prev = (float*)calloc(N, sizeof(float));
    float* u_curr = (float*)calloc(N, sizeof(float));
    float* u_next = (float*)calloc(N, sizeof(float));
    
    // CFL Condition check
    float lambda_x = (cfg.c_speed * cfg.dt) / cfg.dx;
    float lambda_y = (cfg.c_speed * cfg.dt) / cfg.dy;
    
    printf("--- 2D Wave Simulation ---\n");
    printf("Grid: %d x %d, Steps: %d\n", cfg.nx, cfg.ny, cfg.nt);
    printf("CFL lambda_x: %f, lambda_y: %f\n", lambda_x, lambda_y);
    printf("Damping: baseline gamma=%f, profile=%d\n", cfg.damping, cfg.damping_profile_type);
    if (lambda_x*lambda_x + lambda_y*lambda_y > 1.0f) {
        printf("WARNING: CFL stability condition violated! (lambda_x^2 + lambda_y^2 > 1)\n");
        // We will continue to let the user see the blow-up, as required by the lab goals.
    }
    
    // Initial conditions are dispatched through the registry callback.
    if (cfg.initial_fn) {
        cfg.initial_fn(u_curr, u_prev, &cfg);
    }
    
    FILE* out_file = fopen(out_filepath, "wb");
    if (!out_file) {
        printf("Error opening %s\n", out_filepath);
        return;
    }
    
    // Output initial frame
    if (cfg.output_freq > 0) {
        write_frame(out_file, u_curr, cfg.nx, cfg.ny);
    }
    
    int center_i = cfg.nx / 2;
    int center_j = cfg.ny / 2;
    for (int n = 1; n < cfg.nt; n++) {
        // Space loop (interior points) Note Dirichlet BC: boundaries remain 0 from calloc
        for (int i = 1; i < cfg.nx - 1; i++) {
            float x = i * cfg.dx;
            for (int j = 1; j < cfg.ny - 1; j++) {
                int c_idx = IDX(i, j, cfg.ny);
                int l_idx = IDX(i-1, j, cfg.ny);
                int r_idx = IDX(i+1, j, cfg.ny);
                int u_id  = IDX(i, j-1, cfg.ny);
                int d_idx = IDX(i, j+1, cfg.ny);
                
                float c_speed = cfg.c_speed;
                if (cfg.media_type == 1) {
                    if (x < cfg.x_interface) c_speed = cfg.c_alt;
                } else if (cfg.media_type == 2) {
                    float dist_sq = (x - cfg.lens_x)*(x - cfg.lens_x) + (j * cfg.dy - cfg.lens_y)*(j * cfg.dy - cfg.lens_y);
                    if (dist_sq < cfg.lens_radius*cfg.lens_radius) c_speed = cfg.c_alt;
                }
                
                float local_lx2 = (c_speed * cfg.dt / cfg.dx) * (c_speed * cfg.dt / cfg.dx);
                float local_ly2 = (c_speed * cfg.dt / cfg.dy) * (c_speed * cfg.dt / cfg.dy);
                float gamma = damping_at(&cfg, x, j * cfg.dy);
                float gamma_dt_half = 0.5f * gamma * cfg.dt;
                
                u_next[c_idx] = (2.0f * u_curr[c_idx] - (1.0f - gamma_dt_half) * u_prev[c_idx] +
                                local_lx2 * (u_curr[r_idx] - 2.0f * u_curr[c_idx] + u_curr[l_idx]) +
                                local_ly2 * (u_curr[d_idx] - 2.0f * u_curr[c_idx] + u_curr[u_id])) /
                                (1.0f + gamma_dt_half);
                                
                // Time-dependent forcing (applied at center by default)
                if (cfg.forcing_fn && i == center_i && j == center_j) {
                    float t = n * cfg.dt;
                    float src = cfg.forcing_fn(t, &cfg);
                    u_next[c_idx] += c_speed * c_speed * cfg.dt * cfg.dt * src;
                }
            }
        }
        
        // Output frame based on frequency
        if (cfg.output_freq > 0 && n % cfg.output_freq == 0) {
            write_frame(out_file, u_next, cfg.nx, cfg.ny);
        }
        
        // Pointer swap to avoid memory copies
        float* temp = u_prev;
        u_prev = u_curr;
        u_curr = u_next;
        u_next = temp;
    }
    
    fclose(out_file);
    free(u_prev);
    free(u_curr);
    free(u_next);
    printf("Simulation Complete. Output saved to %s\n", out_filepath);
}
