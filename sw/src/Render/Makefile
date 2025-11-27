TARGET = main
CC = gcc
CFLAGS = -Wall -Wextra -g -I../../../mpsLibC/common -I/opt/homebrew/include -I../inc
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL

# Rendering mode: set USE_COLOR_FRAME=1 to use colors struct, otherwise uses pressure/velocity
USE_COLOR_FRAME ?= 0

ifeq ($(USE_COLOR_FRAME),1)
    CFLAGS += -DUSE_COLOR_FRAME
endif

SRC = src/main.c src/Renderer.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJ) $(TARGET)

