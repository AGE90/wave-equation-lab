import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

def read_c_bin_output(filepath, nx, ny, nt, out_freq):
    """
    Reads the binary output from the C simulator.
    """
    num_frames = ((nt - 1) // out_freq) + 1 if out_freq > 0 else 0
    if num_frames == 0:
        return np.empty((0, nx, ny))
        
    try:
        data = np.fromfile(filepath, dtype=np.float32)
        # Reshape into (time, x, y)
        expected_size = num_frames * nx * ny
        if data.size != expected_size:
            print(f"Warning: Expected {expected_size} floats, got {data.size}. Truncating/padding.")
            # Adjust num_frames based on actual size
            actual_frames = data.size // (nx * ny)
            data = data[:actual_frames * nx * ny]
            data = data.reshape((actual_frames, nx, ny))
        else:
            data = data.reshape((num_frames, nx, ny))
        return data
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        return None

def animate_heatmaps(data_frames, out_mp4="wave_simulation.mp4"):
    """
    Generates an MP4 animation from a 3D numpy array (time, x, y).
    Note: matplotlib.animation can be slow for large meshes.
    Saving to mp4 using ffmpeg is prioritized.
    """
    if data_frames is None or data_frames.size == 0:
        print("No data to animate.")
        return
        
    fig, ax = plt.subplots()
    # Find global min/max for color scaling
    # Use a tighter fixed boundary (e.g. 10% of global max or a percentile) to make faint ripples visible
    vmax = np.max(np.abs(data_frames)) * 0.5  # Cap the max brightness to enhance visible contrast of the ripples
    
    # We display transpose so x is horizontal, y is vertical if desired,
    # but standard imshow is (row, col) => (x, y) generally maps X to rows.
    cax = ax.imshow(data_frames[0], vmin=-vmax, vmax=vmax, cmap='seismic', origin='lower')
    fig.colorbar(cax)
    
    title = ax.set_title("Wave Equation Frame 0")
    
    def update(frame_idx):
        cax.set_data(data_frames[frame_idx])
        title.set_text(f"Wave Equation Frame {frame_idx}")
        return cax, title
        
    ani = animation.FuncAnimation(fig, update, frames=len(data_frames), blit=False)
    
    try:
        print(f"Saving animation to {out_mp4} using ffmpeg...")
        writer = animation.FFMpegWriter(fps=30, metadata=dict(artist='WaveLab'), bitrate=1800)
        ani.save(out_mp4, writer=writer)
        print("Successfully saved animation.")
    except Exception as e:
        print(colored_warning("FFmpeg might not be installed or reachable in PATH."))
        print("Details:", e)
        try:
            out_gif = out_mp4.replace(".mp4", ".gif")
            print(f"Saving animation to {out_gif} using Pillow...")
            ani.save(out_gif, writer='pillow', fps=30)
            print("Successfully saved GIF animation.")
        except Exception as gif_err:
            print(colored_warning("Pillow saving also failed."))
            print("Details:", gif_err)
            print("Falling back to standard matplotlib display (WARNING: will block execution).")
            # plt.show() # Uncomment to show directly if Pillow fails.

import json

def colored_warning(msg):
    return f"\033[93m{msg}\033[0m"

def parse_config(filepath):
    config = {}
    try:
        with open(filepath, 'r') as f:
            config = json.load(f)
    except Exception as e:
        print(f"Error reading JSON config {filepath}: {e}")
    return config

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Animate Wave Simulation Binary Output")
    parser.add_argument("--config", type=str, default=None, help="Path to config text file (e.g., experiment01.txt)")
    parser.add_argument("--input", type=str, default="../shared/data/output.bin", help="Path to input .bin file")
    parser.add_argument("--nx", type=int, default=100, help="Grid size X")
    parser.add_argument("--ny", type=int, default=100, help="Grid size Y")
    parser.add_argument("--nt", type=int, default=400, help="Total timesteps")
    parser.add_argument("--freq", type=int, default=10, help="Output frequency (out_freq)")
    parser.add_argument("--output", type=str, default="wave_simulation.mp4", help="Output MP4 file path")
    args = parser.parse_args()
    
    if args.config:
        cfg = parse_config(args.config)
        if 'nx' in cfg: args.nx = int(cfg['nx'])
        if 'ny' in cfg: args.ny = int(cfg['ny'])
        if 'nt' in cfg: args.nt = int(cfg['nt'])
        if 'output_freq' in cfg: args.freq = int(cfg['output_freq'])
    
    print(f"Reading data from {args.input} with shape ({args.nt // args.freq if args.freq > 0 else 0}, {args.nx}, {args.ny})...")
    data = read_c_bin_output(args.input, args.nx, args.ny, args.nt, args.freq)
    if data is not None and data.size > 0:
        animate_heatmaps(data, args.output)
    else:
        print("Failed to load wave data.")

