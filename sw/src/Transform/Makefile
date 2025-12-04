PROJECT = transformTest
BUILD_DIR = build
TARGET = $(PROJECT).elf

CC = gcc

LIB_DIR = ../../mpsLibC
LIB = $(LIB_DIR)/build/mpsLibC.a

OS_TYPE := $(shell uname -s)

C_FLAGS = \
	-Wextra \
	-Wall \
	-O0 \
	-g3

LD_FLAGS = \
	-lm \
	-lpthread \
	-lglfw	\
	

SRC_FILES = \
	src/Transform.c \
	./src/FrameColors_Utils.c	\
	./src/FrameRaw_Utils.c	\
	./src/HelperUtils.c	\
	../Render/src/Renderer.c	\
	test/TestXForm.c

INCLUDES = \
	-I$(LIB_DIR)/common \
	-I$(LIB_DIR)/RingBuffer/PFifo \
	-I$(LIB_DIR)/RingBuffer/LFfifo

ifeq ($(OS_TYPE),Darwin)
LD_FLAGS += \
	-framework OpenGL \
	-L/opt/homebrew/lib

C_FLAGS += \
	-I/opt/homebrew/include
endif

ifeq ($(OS_TYPE),Linux)
LD_FLAGS += -lGL
endif

USE_CUDA ?= 0

# Add CUDA compiler and flags
ifeq ($(USE_CUDA),1)
    NVCC = nvcc
    NVCC_FLAGS = -arch=sm_70 -O3 -Xcompiler -fPIC
    CUDA_SRCS = src/FrameColors_Utils.cu
    CUDA_OBJS = $(CUDA_SRCS:.cu=.o)
    C_FLAGS += -DUSE_CUDA
endif

# Add CUDA objects to OBJS
OBJS += $(CUDA_OBJS)

# Add CUDA compilation rule
ifeq ($(USE_CUDA),1)
%.o: %.cu
	$(NVCC) $(NVCC_FLAGS) -I$(INC_DIR) -I../../Render/inc -I../../Sim/inc -c $< -o $@
endif

# Add CUDA libraries to linker
ifeq ($(USE_CUDA),1)
    LD_FLAGS += -lcudart
endif

.PHONY: all clean

all: $(TARGET)

# Build library first
$(LIB):
	$(MAKE) -C $(LIB_DIR)

# Build executable
$(TARGET): $(SRC_FILES) $(LIB) | $(BUILD_DIR)
	$(CC) -o $@ $(SRC_FILES) $(INCLUDES) $(C_FLAGS) $(LIB) $(LD_FLAGS)

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean
