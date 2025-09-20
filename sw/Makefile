PROJECT = FuildSim
BUILD_DIR = build
ELF_NAME = $(BUILD_DIR)/$(PROJECT).exe

CC = gcc

C_FLAGS = \
		 -Wextra	\
		 -Wall	\
		 -O0	\

LD_FLAGS = \
		  -lm	\
		  -lpthread	\


SRC_FILES = \
		   src/main.c	\

INCLUDES = \
		   -Ilib/common	\

all: $(ELF_NAME)
	

$(ELF_NAME) : $(SRC_FILES) | $(BUILD_DIR)
	$(CC) $(C_FLAGS) -o $@ $(SRC_FILES) $(LD_FLAGS) $(INCLUDES)

$(BUILD_DIR):
	mkdir -p $@

.PHONY : clean
clean : 
	rm -rf ./$(BUILD_DIR)


