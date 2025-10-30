PROJECT = FluidSim
BUILD_DIR = build
TARGET = $(PROJECT).elf

CC = gcc

LIB_DIR = mpsLibC
LIB = $(LIB_DIR)/build/$(LIB_DIR).a 

C_FLAGS = \
		 -Wextra	\
		 -Wall	\
		 -O0	\
		 -g3 	\
		 -I/opt/homebrew/include \

LD_FLAGS = \
			-L/opt/homebrew/lib \
		  -lm	\
		  -lpthread	\
		  -lglfw \
    	-framework OpenGL \

SRC_FILES = \
		   src/main.c	\
		   src/Sim/src/Controller.c	\
		   src/Sim/src/Sim.c	\
		   src/Renderer.c \
		   

INCLUDES = \
		   -ImpsLibC/common	\
		   -ImpsLibC/RingBuffer/PFifo	\


OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

.PHONY : clean all

all: $(TARGET)
	
# Ensure library is built first
$(LIB):
	$(MAKE) -C $(LIB_DIR) all

$(TARGET) : $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@  $(LD_FLAGS) 

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean : 
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)
	$(MAKE) -C $(LIB_DIR) clean


