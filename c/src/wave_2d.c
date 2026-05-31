#include "wave.h"

// Initialize with a Gaussian pulse in the center
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
    if (lambda_x*lambda_x + lambda_y*lambda_y > 1.0f) {
        printf("WARNING: CFL stability condition violated! (lambda_x^2 + lambda_y^2 > 1)\n");
        // We will continue to let the user see the blow-up, as required by the lab goals.
    }
    
    // Initial Conditions
    if (cfg.source_type == 1) {
        // Zero initial displacement for the Ricker wavelet configuration (calloc already zeroed it)
    } else {
        init_gaussian(u_curr, cfg.nx, cfg.ny, cfg.dx, cfg.dy);
        init_gaussian(u_prev, cfg.nx, cfg.ny, cfg.dx, cfg.dy); 
        // u_prev == u_curr implies zero initial velocity
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
                
                u_next[c_idx] = 2.0f * u_curr[c_idx] - u_prev[c_idx] +
                                local_lx2 * (u_curr[r_idx] - 2.0f * u_curr[c_idx] + u_curr[l_idx]) +
                                local_ly2 * (u_curr[d_idx] - 2.0f * u_curr[c_idx] + u_curr[u_id]);
                                
                // Add Ricker wavelet point source in the center
                if (cfg.source_type == 1) {
                    int center_i = (cfg.nx - 1) / 2 - 1;
                    int center_j = (cfg.ny - 1) / 2 - 1;
                    if (i == center_i && j == center_j) {
                        float t = n * cfg.dt;
                        float f = 30.0f;
                        float pi = 3.1415926535f;
                        float pi2_f2_t2 = pi * pi * f * f * t * t;
                        float ricker = (1.0f - 2.0f * pi2_f2_t2) * expf(-pi2_f2_t2);
                        u_next[c_idx] += c_speed * c_speed * cfg.dt * cfg.dt * ricker;
                    }
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
