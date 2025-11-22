UNAME_S := $(shell uname -s)

CFLAGS = \
    -O0 \
    -g3 \
    -Wall \
    -Wextra \

INCLUDES = \
    -I../../mpsLibC/common \
    -I/opt/homebrew/include \

LD_FLAGS = \
    -lpthread \
    -lm \
    -lglfw \
    -L/opt/homebrew/lib \

MACOS_FLAGS =

ifeq ($(UNAME_S),Linux)
    LD_FLAGS += -lGL
endif

ifeq ($(UNAME_S),Darwin)
    MACOS_FLAGS += -framework OpenGL
endif


# ---- Build Rules ----

SRC = main.c
OBJ = $(SRC:.c=.o)


all: $(SRC)
	gcc $(SRC) $(CFLAGS) $(INCLUDES) $(LD_FLAGS) $(MACOS_FLAGS)

clean:
	rm -f $(OBJ) $(TARGET)


