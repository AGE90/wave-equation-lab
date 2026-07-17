import numpy as np

def init_gaussian(nx, ny, dx, dy):
    x0, y0 = (nx * dx) / 2.0, (ny * dy) / 2.0
    sigma = 0.1
    x = np.linspace(0, nx*dx, nx, endpoint=False)
    y = np.linspace(0, ny*dy, ny, endpoint=False)
    X, Y = np.meshgrid(x, y, indexing='ij')
    dist_sq = (X - x0)**2 + (Y - y0)**2
    return np.exp(-dist_sq / (sigma**2))

def build_damping_grid(nx, ny, dx, dy, damping=0.0, damping_profile=None):
    """Return the spatial attenuation field gamma(x, y)."""
    if damping < 0.0:
        raise ValueError("damping must be non-negative")

    profile = damping_profile or {"type": "none"}
    profile_type = profile.get("type", "none")
    gamma = np.full((nx, ny), damping, dtype=np.float32)
    x = np.arange(nx, dtype=np.float32) * dx
    y = np.arange(ny, dtype=np.float32) * dy

    if profile_type == "none":
        return gamma
    if profile_type == "x_split":
        damping_alt = profile.get("damping_alt", damping)
        x_limit = profile.get("x_limit", 0.0)
        if damping_alt < 0.0:
            raise ValueError("damping_alt must be non-negative")
        gamma[x < x_limit, :] = damping_alt
    elif profile_type == "circular_region":
        damping_alt = profile.get("damping_alt", damping)
        radius = profile.get("radius", 0.0)
        if damping_alt < 0.0 or radius <= 0.0:
            raise ValueError("circular_region requires non-negative damping_alt and radius > 0")
        X, Y = np.meshgrid(x, y, indexing="ij")
        inside = (X - profile.get("x", 0.0)) ** 2 + (Y - profile.get("y", 0.0)) ** 2 < radius ** 2
        gamma[inside] = damping_alt
    elif profile_type == "absorbing_boundary":
        width = profile.get("width", 0.0)
        edge_damping = profile.get("edge_damping", damping)
        power = profile.get("power", 2.0)
        if width <= 0.0 or edge_damping < 0.0 or power < 0.0:
            raise ValueError("absorbing_boundary requires width > 0 and non-negative edge_damping and power")
        X, Y = np.meshgrid(x, y, indexing="ij")
        distance_to_edge = np.minimum.reduce((X, x[-1] - X, Y, y[-1] - Y))
        penetration = np.clip(1.0 - distance_to_edge / width, 0.0, 1.0)
        gamma = damping + (edge_damping - damping) * penetration ** power
    else:
        raise ValueError("damping_profile.type must be none, x_split, circular_region, or absorbing_boundary")

    return gamma


def solve_2d(nx, ny, nt, dx, dy, dt, c_speed, c_alt=-1.0, x_interface=0.0, source_type=0, media_type=0, lens_radius=0.0, lens_x=0.0, lens_y=0.0, damping=0.0, damping_profile=None):
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
        
    gamma = build_damping_grid(nx, ny, dx, dy, damping, damping_profile)
    gamma_dt_half = 0.5 * gamma[1:-1, 1:-1] * dt
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
            
        u[n+1, 1:-1, 1:-1] = ((2 * u[n, 1:-1, 1:-1] -
                    (1 - gamma_dt_half) * u[n-1, 1:-1, 1:-1] +
                    lx2_inner * (u[n, 2:, 1:-1] - 2*u[n, 1:-1, 1:-1] + u[n, :-2, 1:-1]) +
                    ly2_inner * (u[n, 1:-1, 2:] - 2*u[n, 1:-1, 1:-1] + u[n, 1:-1, :-2])) /
                       (1 + gamma_dt_half))
                              
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
