import numpy as np

def init_gaussian(nx, ny, dx, dy):
    x0, y0 = (nx * dx) / 2.0, (ny * dy) / 2.0
    sigma = 0.1
    x = np.linspace(0, nx*dx, nx, endpoint=False)
    y = np.linspace(0, ny*dy, ny, endpoint=False)
    X, Y = np.meshgrid(x, y, indexing='ij')
    dist_sq = (X - x0)**2 + (Y - y0)**2
    return np.exp(-dist_sq / (sigma**2))

def solve_2d(nx, ny, nt, dx, dy, dt, c_speed, c_alt=-1.0, x_interface=0.0, source_type=0, media_type=0, lens_radius=0.0, lens_x=0.0, lens_y=0.0):
    """
    Solves the 2D wave equation and returns an array of shape (nt, nx, ny).
    Used for prototyping and comparison with C.
    """
    if media_type == 0 and c_alt > 0.0:
        media_type = 1
        
    is_ref = (media_type > 0)
    
    if is_ref:
        x = np.linspace(0, nx*dx, nx, endpoint=False)
        if media_type == 1:
            c_speed_grid = np.where(x < x_interface, c_alt, c_speed).reshape(nx, 1)
        elif media_type == 2:
            y = np.linspace(0, ny*dy, ny, endpoint=False)
            X, Y = np.meshgrid(x, y, indexing='ij')
            dist_sq = (X - lens_x)**2 + (Y - lens_y)**2
            c_speed_grid = np.where(dist_sq < lens_radius**2, c_alt, c_speed)
        
        lx2 = (c_speed_grid * dt / dx)**2
        ly2 = (c_speed_grid * dt / dy)**2
    else:
        lambda_x = (c_speed * dt) / dx
        lambda_y = (c_speed * dt) / dy
        
        if lambda_x**2 + lambda_y**2 > 1.0:
            print("WARNING: CFL stability condition violated!")
            
        lx2 = lambda_x**2
        ly2 = lambda_y**2
        
    u = np.zeros((nt, nx, ny), dtype=np.float32)
    
    # Initial Conditions
    if source_type != 1:
        u[0] = init_gaussian(nx, ny, dx, dy)
        u[1] = np.copy(u[0]) # Zero initial velocity approximation
    
    for n in range(1, nt - 1):
        if is_ref:
            # We must use lx2 and ly2 which might broadcast differently if lx2 is (nx, 1)
            # lx2 is shape (nx, 1). We need to slice it for interior points.
            lx2_inner = lx2[1:-1, :]
            ly2_inner = ly2[1:-1, :]
        else:
            lx2_inner = lx2
            ly2_inner = ly2
            
        u[n+1, 1:-1, 1:-1] = (2 * u[n, 1:-1, 1:-1] - u[n-1, 1:-1, 1:-1] +
                              lx2_inner * (u[n, 2:, 1:-1] - 2*u[n, 1:-1, 1:-1] + u[n, :-2, 1:-1]) +
                              ly2_inner * (u[n, 1:-1, 2:] - 2*u[n, 1:-1, 1:-1] + u[n, 1:-1, :-2]))
                              
        if source_type == 1:
            center_i = (nx - 1) // 2 - 1
            center_j = (ny - 1) // 2 - 1
            t = n * dt
            f = 30.0
            pi2_f2_t2 = (np.pi**2) * (f**2) * (t**2)
            ricker = (1.0 - 2.0 * pi2_f2_t2) * np.exp(-pi2_f2_t2)
            if is_ref:
                c_local = c_speed_grid[center_i, 0]
            else:
                c_local = c_speed
            u[n+1, center_i, center_j] += (c_local**2) * (dt**2) * ricker
            
    return u
