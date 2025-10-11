PROJECT = FuildSim
BUILD_DIR = build
TARGET = $(PROJECT).elf

CC = gcc

C_FLAGS = \
		 -Wextra	\
		 -Wall	\
		 -O0	\
		 -g 	\

LD_FLAGS = \
		  -lm	\
		  -lpthread	\

SRC_FILES = \
		   src/main.c	\

INCLUDES = \
		   -ImpsLibC/common	\


OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

all: $(TARGET)
	
$(TARGET) : $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@  $(LD_FLAGS) 

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

.PHONY : clean
clean : 
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TARGET)


