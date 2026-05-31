import numpy as np
from src.wave.solver_2d import solve_2d

u = solve_2d(nx=301, ny=301, nt=1001, dx=0.00333333, dy=0.00333333, dt=0.0005, c_speed=4.0)
print("Python solve_2d completed. Output shape:", u.shape)
