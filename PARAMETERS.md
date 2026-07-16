# Parameter Reference

This document is the single source of truth for simulation configuration parameters.
It is designed to be extended as new parameters are added.

## 1. Config Shape

All simulations read a JSON file from `shared/configs/*.json`.

Minimal example:

```json
{
  "nx": 300,
  "ny": 300,
  "nt": 3000,
  "dx": 0.00333333,
  "dy": 0.00333333,
  "dt": 0.0005,
  "c": 3.0,
  "output_freq": 10
}
```

## 2. Parameter Catalog

### 2.1 Grid And Time

| Name | Type | Required | Default | Description |
|---|---|---|---|---|
| `nx` | int | Yes | `0` | Number of grid cells along x. |
| `ny` | int | Yes | `0` | Number of grid cells along y. |
| `nt` | int | Yes | `0` | Number of time steps. |
| `dx` | float | Yes | `0.0` | Spatial step in x. |
| `dy` | float | Yes | `0.0` | Spatial step in y. |
| `dt` | float | Yes | `0.0` | Time step size. |

### 2.2 Wave Speed And Medium

| Name | Type | Required | Default | Description |
|---|---|---|---|---|
| `c` | float | Yes | `0.0` | Background wave speed. |
| `media_type` | int | No | `0` | Medium model: `0` homogeneous, `1` split interface, `2` circular lens. |
| `c_alt` | float | No | `-1.0` | Alternate wave speed used by non-homogeneous media. |
| `x_limit` | float | No | `0.0` | Interface position for `media_type=1` (`x < x_limit` uses `c_alt`). |
| `lens_radius` | float | No | `0.0` | Lens radius for `media_type=2`. |
| `lens_x` | float | No | `0.0` | Lens center x for `media_type=2`. |
| `lens_y` | float | No | `0.0` | Lens center y for `media_type=2`. |

Compatibility behavior:
- If `media_type` is omitted and `c_alt > 0`, the loader auto-selects `media_type=1`.

### 2.3 Initial Condition

| Name | Type | Required | Default | Description |
|---|---|---|---|---|
| `initial_type` | string or int | No | `"zero"` | Initial state selector. |

Accepted values:
- String: `"gaussian"`, `"point_impulse"`, `"zero"`
- String aliases: `"impulse"` -> `"point_impulse"`, `"none"` -> `"zero"`
- Int: `0` gaussian, `1` point impulse, `2` zero

### 2.4 Time-Dependent Forcing

The preferred format is a nested `forcing` object.

| Name | Type | Required | Default | Description |
|---|---|---|---|---|
| `forcing` | object | No | omitted | Forcing configuration block. |
| `forcing.type` | string or int | No | `"none"` | Forcing selector. |
| `forcing.f` | float | No | `30.0` | Main frequency parameter. Used by ricker/sine/chirp. |
| `forcing.f1` | float | No | `60.0` | End frequency for chirp. |
| `forcing.t1` | float | No | `1.0` | Sweep duration for chirp. |

Accepted `forcing.type` values:
- String: `"none"`, `"ricker"`, `"sine"`, `"chirp"`
- Int: `0` none, `1` ricker, `2` sine, `3` chirp

Compatibility behavior:
- Legacy top-level `forcing_type` (int) is still accepted.
- Current C loader reads `forcing.f` for chirp start frequency. `forcing.f0` is not consumed by the loader.

Examples:

```json
"forcing": { "type": "ricker", "f": 30.0 }
```

```json
"forcing": { "type": "sine", "f": 20.0 }
```

```json
"forcing": { "type": "chirp", "f": 5.0, "f1": 50.0, "t1": 1.0 }
```

### 2.5 Output

| Name | Type | Required | Default | Description |
|---|---|---|---|---|
| `output_freq` | int | No | `0` | Write a frame every N steps. If `<= 0`, no frames are written. |

## 3. Practical Stability Constraint

For 2D explicit finite differences, stability requires:

$$
\left(\frac{c \cdot dt}{dx}\right)^2 + \left(\frac{c \cdot dt}{dy}\right)^2 \le 1
$$

If violated, the simulation may blow up numerically.

## 4. Extension Template For New Parameters

When adding a new parameter, append a row to the relevant section and include:

| Field | What to document |
|---|---|
| Name | Exact JSON key |
| Type | `int`, `float`, `string`, `object`, etc. |
| Required | Yes/No |
| Default | Runtime default used by loader |
| Valid values | Enum/range/constraints |
| Scope | Which mode or media/forcing type it applies to |
| Version note | Optional compatibility/deprecation note |

Recommended process:
1. Add loader support in C (`c/src/io.c`) and any Python loader path.
2. Add/adjust runtime behavior in solver modules.
3. Add at least one example config in `shared/configs`.
4. Update this file in the same change.
