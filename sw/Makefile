# === Configuration ===
ON_REMOTE := false

PROJECT = FluidSim
BUILD_DIR = build
TARGET = $(PROJECT).elf

OS_TYPE := $(shell uname -s)

# Default compiler
ifeq ($(ON_REMOTE),true)
    COMPILER = nvcc
    LIB_DIR_CK = ./lib/ck/build/lib
    LIB_CK = $(LIB_DIR_CK)/libck.a
else
    COMPILER = gcc
endif

LIB_DIR_MPS = mpsLibC
LIB_MPS = $(LIB_DIR_MPS)/build/$(LIB_DIR_MPS).a

# === Flags ===
C_FLAGS = -Wextra -Wall -O0 -g3 -MMD -MP
LD_FLAGS = -lm -lpthread

NVCC_FLAGS = -O2 -std=c++14 --expt-extended-lambda -arch=sm_70

# === Source files ===
SRC_C_FILES = \
    src/main.c \
    src/Sim/src/Sim.c \
    src/Sim/src/Controller.c \
    src/Sim/src/Advection.c \
    src/Sim/src/PressureSolver.c \
    src/Sim/src/SimDebugUtils.c \
    src/Transform/src/Transform.c \
    src/Transform/src/FrameColors_Utils.c \
    src/Transform/src/FrameRaw_Utils.c \
    src/Transform/src/HelperUtils.c \
    src/Render/src/Renderer.c \
    src/Render/src/Renderer_ColorFrame.c \
    src/Render/src/Renderer_RawFrames.c \
    src/Render/src/Renderer_Display.c \
    src/Render/src/Renderer_ColorFrame_OS.c

SRC_CUDA_FILES =  # will only be used if ON_REMOTE=true

ifeq ($(OS_TYPE),Darwin)
    LD_FLAGS += -framework OpenGL -L/opt/homebrew/lib
    C_FLAGS += -I/opt/homebrew/include
    SRC_C_FILES += src/Render/src/Renderer_OffScreen_Stub.c
endif

ifeq ($(OS_TYPE),Linux)
    LD_FLAGS += -lEGL -lGL
    SRC_C_FILES += src/Render/src/Renderer_OffScreen.c

    ifeq ($(ON_REMOTE),true)
        SRC_CUDA_FILES += \
            src/Transform/src/FrameColors_Utils_Device.cu \
            src/Sim/cuda_src/Advection_cuda.cu \
            src/Sim/cuda_src/Pressure_cuda.cu
    else
        LD_FLAGS += -lglfw -lGLEW
    endif
endif

# Combine all sources for dependency generation
SRC_FILES = $(SRC_C_FILES) $(SRC_CUDA_FILES)

# Object files
OBJS_C = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_C_FILES))
OBJS_CUDA = $(patsubst src/%.cu,$(BUILD_DIR)/%.o,$(SRC_CUDA_FILES))
OBJS = $(OBJS_C) $(OBJS_CUDA)

DEPS = $(OBJS:.o=.d)

# === Include paths ===
INCLUDES = -I$(LIB_DIR_MPS)/common \
           -I$(LIB_DIR_MPS)/RingBuffer/PFifo \
           -I$(LIB_DIR_MPS)/RingBuffer/LFfifo \
           -I./lib/ck/build/include

HDR_FILES = src/config.h

# === Targets ===
.PHONY: all clean

all: $(TARGET)

# Ensure libraries are built
$(LIB_MPS):
	$(MAKE) -C $(LIB_DIR_MPS) all

$(TARGET): $(OBJS) $(LIB_MPS) $(LIB_CK) | $(BUILD_DIR)
	$(CC) -o $@ $(OBJS) $(LIB_MPS) $(LIB_CK) $(LD_FLAGS)

# Compile C files
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

# Compile CUDA files
$(BUILD_DIR)/%.o: src/%.cu | $(BUILD_DIR)
	mkdir -p $(dir $@)
	nvcc $(NVCC_FLAGS) $(INCLUDES) -c $< -o $@

# Include dependency files
-include $(DEPS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $@

# Clean
clean:
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(LIB_DIR_MPS) clean
