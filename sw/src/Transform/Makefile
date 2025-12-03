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
	-lGL

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
