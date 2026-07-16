# ==============================================================================
# Wave Equation Simulator Makefile
# 
# Main commands:
#   make                   - Compiles the C code, runs the 'default' experiment,
#                            and generates the visualization (default.gif).
#   make EXP=<name>        - Runs a specific experiment (e.g., make EXP=experiment01).
#                            Requires shared/configs/<name>.json to exist.
#   make build             - Only compiles the C simulation engine.
#   make clean             - Removes compiled object files, binaries and generated data.
#   make clean_outputs     - Removes generated GIFs and MP4s in the output folder.
# ==============================================================================

EXP ?= default
CONFIG = shared/configs/$(EXP).json
DATA = shared/data/$(EXP).bin
OUTPUT = output/$(EXP).gif

# C Compilation settings
CC = gcc
CFLAGS = -Wall -O3
CPPFLAGS = -I$(INC_DIR)
LDFLAGS = -lm

SRC_DIR = c/src
OBJ_DIR = c/build
INC_DIR = c/include

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

TARGET = $(OBJ_DIR)/wave_sim

.PHONY: all build run clean

all: run

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: build
	@echo "=== Running C Simulation ($(EXP)) ==="
	./$(TARGET) $(CONFIG) $(DATA)
	@echo "=== Rendering Python Visualization ($(EXP)) ==="
	cd python && uv run python src/wave/visualization.py --config ../$(CONFIG) --input ../$(DATA) --output ../$(OUTPUT)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	rm -f shared/data/*.bin

clean_outputs:
	rm -f output/*.gif output/*.mp4
