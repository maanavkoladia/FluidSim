ON_REMOTE:=true

PROJECT = FluidSim
BUILD_DIR = build
TARGET = $(PROJECT).elf

COMPILER = gcc
LIB_DIR_MPS = mpsLibC
LIB_MPS = $(LIB_DIR_MPS)/build/$(LIB_DIR_MPS).a

ifeq ($(ON_REMOTE),true) 
COMPILER = nvcc
LIB_DIR_CK = ./lib/ck/build/lib
LIB_CK = $(LIB_DIR_CK)/libck.a
else
COMPILER = gcc

endif


OS_TYPE := $(shell uname -s)

C_FLAGS = \
	-Wextra \
	-Wall \
	-O0 \
	-g3 \
	-MMD \
	-MP

LD_FLAGS = \
	-lm \
	-lpthread \

NVCC_FLAGS = \
			 -O2	\
			 -std=c++14 \
			 --expt-extended-lambda \
			 -arch=sm_70

SRC_FILES = \
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

INCLUDES = \
	-I$(LIB_DIR_MPS)/common \
	-I$(LIB_DIR_MPS)/RingBuffer/PFifo \
	-I$(LIB_DIR_MPS)/RingBuffer/LFfifo \
	-I./lib/ck/build/include

HDR_FILES = \
	src/config.h

ifeq ($(OS_TYPE),Darwin)
LD_FLAGS += \
	-framework OpenGL \
	-L/opt/homebrew/lib
C_FLAGS += \
	-I/opt/homebrew/include
SRC_FILES += \
	src/Render/src/Renderer_OffScreen_Stub.c
endif

ifeq ($(OS_TYPE),Linux)
    # Common Linux flags
    LD_FLAGS += -lEGL -lGL

    SRC_FILES += \
        src/Render/src/Renderer_OffScreen.c


    # Only link GLFW/GLEW if not on remote
    ifeq ($(ON_REMOTE),false)
        LD_FLAGS += -lglfw -lGLEW
    else
        SRC_FILES += src/Transform/src/FrameColors_Utils_Device.cu 	\
					 ./src/Sim/cuda_src/Advection_cuda.c	\
					 ./src/Sim/cuda_src/Pressure_cuda.c		\

C_FLAGS += $(NVCC_FLAGS)

    endif
endif

OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
DEPS = $(OBJS:.o=.d)

.PHONY: clean all

all: $(TARGET)

# Ensure library is built first
$(LIB_MPS):
	$(MAKE) -C $(LIB_DIR_MPS) all

$(TARGET): $(OBJS) $(LIB_MPS) $(LIB_CK) | $(BUILD_DIR)
	$(CC) -o $@ $(OBJS) $(LIB_MPS) $(LIB_CK) $(LD_FLAGS)

# Compile object files and generate dependency files
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

# Include dependency files if they exist
-include $(DEPS)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(LIB_DIR_MPS) clean
