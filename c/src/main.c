#include "wave.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <config_filepath> <output_bin_filepath>\n", argv[0]);
        return 1;
    }
    
    Config cfg = load_config(argv[1]);
    run_wave_2d(cfg, argv[2]);
    return 0;
}
