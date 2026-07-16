# Code Architecture & Experimentation Guide

This guide provides deep technical details on how the Wave Equation Lab is implemented under the hood, how the C and Python environments communicate, and how you can run your own custom simulation experiments.

---

## 1. How This Project Works at the Code Level

The project uses two different programming paradigms to solve the identical mathematical problem (the discrete explicit finite-difference 2D wave equation). 

### 1.1 The C Implementation: Low-Level Optimization
The C code in `c/src/wave_2d.c` is built for maximum execution speed and minimal memory overhead.

- **Flat Memory Arrays**: Multidimensional arrays are notoriously slow if allocated via arrays of pointers. In C, life is kept in 1D. A $N_x \times N_y$ grid is allocated as a contiguous 1D array of floats: `float* u = calloc(nx * ny, sizeof(float))`. 
- **Index Mapping Macro**: To access a 2D coordinate `(i, j)`, the engine uses an inline macro: `IDX(i, j, ny) = (i * ny) + j`.
- **Zero-Copy Pointer Swapping**: The finite difference method needs past (`u_prev`), current (`u_curr`), and future (`u_next`) states. In C, moving arrays is expensive. Instead, at the end of each timestep, the engine merely swaps the pointer addresses (`u_prev = u_curr; u_curr = u_next`). Memory is reused indefinitely, keeping the footprint to exactly $O(N_x N_y)$.
- **Binary I/O**: Instead of writing easily readable but highly inefficient CSVs, the `io.c` file uses `fwrite` to dump raw `float32` byte buffers straight to the disk.

### 1.1.1 Compilation System & Build Orchestrator (The Top-Level Makefile)

To manage the entire lifecycle of the simulation (compilation, C execution, and Python visualization), the project uses a top-level `Makefile` situated at the root directory. 

When you run the commands below, `make` triggers an orchestrated pipeline.

#### Core `make` Commands:

- **`make` (or `make all`)**
  Compiles the C code, runs the simulation using `shared/configs/default.json`, and triggers the Python visualizer to generate `output/default.gif`.

- **`make EXP=<experiment_name>`**
  The primary way to run custom simulations. Replaces the default target with your specified experiment name. 
  *Example:* `make EXP=experiment02` will feed `shared/configs/experiment02.json` into the C engine, dump the binary to `shared/data/experiment02.bin`, and automatically request Python to render `output/experiment02.gif`.

- **`make build`**
  Only checks object files and compiles the C simulation engine (`c/build/wave_sim`) without actually running it.

- **`make clean`**
  Removes all compiled C object (`.o`) files from `c/build/`, deletes the `wave_sim` executable, and clears out any generated `.bin` datasets in `shared/data/` to start fresh.

- **`make clean_outputs`**
  Removes all generated visual artifacts (`.gif` and `.mp4` files) sitting in the `output/` directory.

#### What happens during C Compilation?
When `make` hits the C target:
1. **Directory Creation**: Ensures a dedicated output folder exists via `mkdir -p c/build`.
2. **Compilation**: The compiler translates each source file (`.c`) into an object file (`.o`).
   - `-Wall` enables standard compiler warnings.
   - `-O3` enables aggressive loop and performance optimization (critical for physics constraints).
3. **Linking**: Combines the components with `gcc ... -o c/build/wave_sim -lm`. The `-lm` flag links the C standard Math library, required for analytical evaluations like `expf` inside the math loops.

### 1.2 The Python Implementation: High-Level Vectorization
The Python code in `python/src/wave/solver_2d.py` is built for prototyping, dynamic modeling, and rendering.

- **NumPy Slicing**: Python `for` loops are too slow for millions of grid updates. Instead, the mathematical stencil is applied to the whole grid at once using matrix slicing: `u[n+1, 1:-1, 1:-1] = 2 * u[n, 1:-1, 1:-1] ...` operations delegate the real math to precompiled C/Fortran libraries within NumPy.
- **Dynamic Matrices**: For heterogeneous properties (like changes in the speed of sound $c$), Python uses `numpy.where` to conditionally create multi-dimensional coefficient grids instantly, unlike C which relies on conditional checks within nested `for` loops.

---

## 2. Integration: The Python-C Pipeline

Because Python is best at visualization and C is best at computation, they are integrated via a **shared file system protocol**.

**Data Flow:**
1. **Config parsing**: The C executable reads a JSON file (`configs/*.json`) to load spatial dimensions constraints ($dt, dx, dy, nx, ny, nt$) and physical parameters (wave speed `c`, source type, etc).
2. **Binary Dump**: C solves the wave stencil and incrementally appends `float32` bytes to a raw binary file (`data/output.bin`). Only frames dictated by `output_freq` are written to save disk space.
3. **Python Ingestion**: `visualization.py` uses `np.fromfile(file, dtype=np.float32)`. Since raw binaries contain no metadata, you *must* pass the correct `--nx`, `--ny`, and `--nt` arguments so NumPy knows how to properly reshape the 1D byte array into a 3D `[time, x, y]` video tensor.
4. **Rendering**: Matplotlib wraps the tensor and steps through it with `FuncAnimation`, compiling to `.mp4` using `ffmpeg` or `.gif`.

---

## 3. How to Generate Simulations with Different Parameters

To execute a custom experiment, you manipulate the shared config files. 

### Step 1: Create a Configuration
Create a new file in `shared/configs/my_experiment.json`. The framework is highly parameterized. Here is an example of a configuration detailing a circular slow-wave lens:

```json
{
  "nx": 300,
  "ny": 300,
  "nt": 800,
  "dt": 0.0005,
  "dx": 0.00333,
  "dy": 0.00333,
  "c": 4.0,
  "c_alt": 1.0,
  "media_type": 2,
  "lens_radius": 0.25,
  "lens_x": 0.5,
  "lens_y": 0.5,
  
  "output_freq": 10
}
```

#### Available Parameters:
See `PARAMETERS.md` for the full, continuously maintained parameter catalog (types, defaults, compatibility notes, and extension rules).


### Step 2: Run the High-Speed Simulation (C)
Call the compiled C engine with your custom config and where you want to drop the binary.

```bash
cd c/
./build/wave_sim ../shared/configs/my_experiment.json ../shared/data/my_experiment.bin
```

### Step 3: Render the Video (Python)
Use Python to transform the `.bin` to `.mp4` or `.gif`. By providing the `--config` argument with your JSON configuration file, the visualization script automatically extracts exact matching dimensions (`nx`, `ny`, `nt`, etc.) and sets up the render:

```bash
cd python/
uv run python src/wave/visualization.py \
    --config ../shared/configs/my_experiment.json \
    --input ../shared/data/my_experiment.bin \
    --output ../output/my_custom_wave.mp4
```

Alternatively, you can manually override or define the arguments used for parsing the binary. Notice that the arguments passed into the Python command must exactly match what you put in `my_experiment.json`, as raw binaries contain no metadata. Since `nt=2000` and `output_freq=20`, the script will render $2000 / 20 = 100$ frames.

```bash
cd python/
uv run python src/wave/visualization.py \
    --input ../shared/data/my_experiment.bin \
    --nx 200 --ny 200 --nt 2000 --freq 20 \
    --output ../output/my_custom_wave.gif
```

---

## 4. Guide for Further Experimentation

Here are a few ways to interact with the project code to test different physical and computational behaviors.

### Experiment A: Break the CFL Condition
The stability of a wave solver depends on the **Courant-Friedrichs-Lewy (CFL)** condition. The wave speed on the discrete grid cannot move faster than one unit cell per timestep. 
- **Action**: In your config, arbitrarily increase `c` or `dt`, or shrink `dx` so that $\frac{c \cdot \Delta t}{\Delta x} > 0.707$ (for 2D).
- **Result**: Watch the simulation instantly blow up to `NaN` or infinity within a few frames.

### Experiment B.1: Linear Change of Media (Experiment 02)
The wave encounters a sharp flat boundary where the material properties change.
- **Action**: Use `media_type: 1` in your config file. Specify the boundary location using `x_limit` (e.g. `0.4`), and set the new speed `c_alt` for the region $x < x\_limit$.
- **Result**: You will observe the classic phenomena of reflection and refraction across a straight interface, just like light entering water. Try this by running `make EXP=experiment02`!

### Experiment B.2: Creating "Lenses" (Experiment 03)
We can also create circular regions of differing media.
- **Action**: Use `media_type: 2` in your config file. Then specify `c_alt` as well as `lens_radius`, `lens_x` and `lens_y` to generate a localized spherical area where the wave travels slower or faster.
- **Result**: You will observe the wave front bending and refracting spherically as it passes through the circular anomaly zone, mirroring optical lens principles. Try this by running `make EXP=experiment03`!

### Experiment C: Changing Forcing Functions
Currently, the codebase injects a 'Ricker wavelet' (a seismic wave pulse) into the dead center of the grid over time.
- **Action**: Try changing frequency parameter $f=30.0$ in `wave_2d.c`. Higher frequencies ($f=100.0$) will yield extremely sharp, short ripples, while lower frequencies ($f=5.0$) will yield broad, smooth waves. Or, comment out the Ricker wavelet logic globally and rely purely on the Gaussian initial condition `init_gaussian()`.

### Experiment D: Spatial Attenuation And Absorbing Layers
The damped wave equation adds $\gamma(x,y)u_t$, which removes energy over time. This represents air absorption, sound transmission through walls, or seismic attenuation in geological material. The C and NumPy solvers both support a uniform baseline coefficient and optional spatial profiles.

For uniform attenuation, add the following to any configuration:

```json
"damping": 0.01
```

For a smooth absorbing layer that minimizes reflection from the zero-valued grid boundaries, use:

```json
"damping": 0.01,
"damping_profile": {
  "type": "absorbing_boundary",
  "width": 0.12,
  "edge_damping": 20.0,
  "power": 2.0
}
```

Run the included experiment with `make EXP=damping_absorbing_boundary`. The attenuation is low in the center and rises smoothly at the edges. `x_split` and `circular_region` profiles can instead model a damped geological layer or acoustic-foam inclusion; see [PARAMETERS.md](PARAMETERS.md) for their fields and constraints.