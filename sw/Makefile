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
		 -O0	\
		 -g3 	\

LD_FLAGS = \
		  -lm	\
		  -lpthread	\
		  -lglfw \

SRC_FILES = \
			src/main.c	\
			src/Sim/src/Sim.c \
			src/Sim/src/Controller.c \
			src/Sim/src/Advection.c	\
			src/Sim/src/PressureSolver.c \
			src/Sim/src/SimDebugUtils.c \
			src/Transform/src/Transform.c \
		   
INCLUDES = \
		   -I$(LIB_DIR)/common	\
		   -I$(LIB_DIR)/RingBuffer/PFifo	\
		   -I$(LIB_DIR)/RingBuffer/LFfifo	\


ifeq ($(OS_TYPE),Darwin)
    LD_FLAGS += \
				-framework OpenGL	\
				-L/opt/homebrew/lib

	C_FLAGS += \
			   -I/opt/homebrew/include 

endif

ifeq ($(OS_TYPE),Linux)
endif

#ifeq($(OS_NAME),Darwin)
#	LD_FLAGS += \
#	-framework OpenGL
#endif

OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

.PHONY : clean all

all: $(TARGET)
	
# Ensure library is built first
$(LIB):
	$(MAKE) -C $(LIB_DIR) all

$(TARGET) : $(OBJS) $(LIB) | $(BUILD_DIR)
	$(CC) -o $@ $(OBJS) $(LIB) $(LD_FLAGS) 

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean : 
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean
