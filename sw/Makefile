PROJECT = FluidSim
BUILD_DIR = build
TARGET = $(PROJECT).elf

CC = gcc

LIB_DIR = mpsLibC
LIB = $(LIB_DIR)/build/$(LIB_DIR).a 


OS_TYPE := $(shell uname -s)

C_FLAGS = \
		 -Wextra	\
		 -Wall	\
		 -O0 	\
		 -g3 	\
		 -MMD \
		 -MP

LD_FLAGS = \
		  -lm	\
		  -lpthread	\
		  -lglfw \
		  -lGL	\
		  -fsanitize=address \

SRC_FILES = \
			src/main.c	\
			src/Sim/src/Sim.c \
			src/Sim/src/Controller.c \
			src/Sim/src/Advection.c	\
			src/Sim/src/PressureSolver.c \
			src/Sim/src/SimDebugUtils.c \
			src/Transform/src/Transform.c \
			src/Transform/src/FrameColors_Utils.c \
			src/Transform/src/FrameRaw_Utils.c \
			src/Transform/src/HelperUtils.c \
			src/Render/src/Renderer.c	\
		   
INCLUDES = \
		   -I$(LIB_DIR)/common	\
		   -I$(LIB_DIR)/RingBuffer/PFifo	\
		   -I$(LIB_DIR)/RingBuffer/LFfifo	\

HDR_FILES = \
		   src/config.h

ifeq ($(OS_TYPE),Darwin)
    LD_FLAGS += \
				-framework OpenGL	\
				-L/opt/homebrew/lib

	C_FLAGS += \
			   -I/opt/homebrew/include 

endif

ifeq ($(OS_TYPE),Linux)
endif

OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))
DEPS = $(OBJS:.o=.d)

.PHONY : clean all

all: $(TARGET)
	
# Ensure library is built first
$(LIB):
	$(MAKE) -C $(LIB_DIR) all

$(TARGET) : $(OBJS) $(LIB) | $(BUILD_DIR)
	$(CC) -o $@ $(OBJS) $(LIB) $(LD_FLAGS) 

# Compile object files and generate dependency files
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

# Include dependency files if they exist
-include $(DEPS)

$(BUILD_DIR):
	mkdir -p $@

clean : 
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean
