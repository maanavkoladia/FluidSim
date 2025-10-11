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
		   -Ilib/common	\

all: $(TARGET)
	

$(TARGET) : $(SRC_FILES) | $(BUILD_DIR)
	$(CC) $(C_FLAGS) -o $@ $(SRC_FILES) $(LD_FLAGS) $(INCLUDES)

$(BUILD_DIR):
	mkdir -p $@

.PHONY : clean
clean : 
	rm -rf ./$(BUILD_DIR)
	rm -rf $(TAR)


