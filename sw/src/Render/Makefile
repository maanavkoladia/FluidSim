TARGET = main
CC = gcc
CFLAGS = -Wall -Wextra -g  -I/opt/homebrew/include -I../inc
INC = -I../../mpsLibC/common/ -I../../mpsLibC/RingBuffer/LFfifo/
SRC = main.c src/Renderer.c
OBJ = $(SRC:.c=.o)
LIB = ../../mpsLibC/build/mpsLibC.a

OS_TYPE := $(shell uname -s)

ifeq ($(OS_TYPE),Darwin)
    LDFLAGS = -framework OpenGL -lglfw
    CFLAGS += -I/opt/homebrew/include
endif

ifeq ($(OS_TYPE),Linux)
    LDFLAGS = -lGL -lglfw
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LIB) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INC)

clean:
	rm -f $(OBJ) $(TARGET)
