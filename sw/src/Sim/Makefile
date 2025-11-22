# -----------------------------
#  Compiler settings
# -----------------------------
CC       := gcc
CFLAGS   := -Wall -Wextra -Werror -O2 -g \
			-Iinc \
			-I/home/karemeldin1/379k_project/FluidSim/sw/mpsLibC/common

LDFLAGS  :=
TARGET   := sim

# -----------------------------
#  Directories
# -----------------------------
SRC_DIR  := src
INC_DIR  := inc
BUILD_DIR := build

# -----------------------------
#  Source / Object files
# -----------------------------
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# -----------------------------
#  Default rule
# -----------------------------
all: $(TARGET)

# -----------------------------
#  Link final executable
# -----------------------------
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# -----------------------------
#  Build object files
# -----------------------------
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------
#  Create build directory if needed
# -----------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# -----------------------------
#  Clean builds
# -----------------------------
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# -----------------------------
#  Phony targets
# -----------------------------
.PHONY: all clean
