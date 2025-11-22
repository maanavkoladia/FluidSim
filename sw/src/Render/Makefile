TARGET = main
CC = gcc
CFLAGS = -Wall -Wextra -g -I../../../mpsLibC/common -I/opt/homebrew/include -I/inc
LDFLAGS = -L/opt/homebrew/lib -lglfw -framework OpenGL

SRC = src/main.c src/Renderer.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJ) $(TARGET)

