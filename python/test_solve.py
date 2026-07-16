import numpy as np

from src.wave.solver_2d import build_damping_grid, solve_2d


def test_zero_damping_matches_default():
	kwargs = dict(nx=31, ny=31, nt=40, dx=0.01, dy=0.01, dt=0.001, c_speed=3.0)
	np.testing.assert_allclose(solve_2d(**kwargs), solve_2d(**kwargs, damping=0.0))


def test_uniform_damping_reduces_late_energy():
	kwargs = dict(nx=51, ny=51, nt=200, dx=0.01, dy=0.01, dt=0.001, c_speed=3.0)
	undamped = solve_2d(**kwargs)
	damped = solve_2d(**kwargs, damping=4.0)
	assert np.sum(damped[-1] ** 2) < np.sum(undamped[-1] ** 2)


def test_absorbing_boundary_profile_increases_at_edges():
	gamma = build_damping_grid(
		11, 11, 0.1, 0.1, damping=0.01,
		damping_profile={"type": "absorbing_boundary", "width": 0.2, "edge_damping": 1.0, "power": 2.0},
	)
	assert gamma[0, 0] == np.float32(1.0)
	assert gamma[5, 5] == np.float32(0.01)
