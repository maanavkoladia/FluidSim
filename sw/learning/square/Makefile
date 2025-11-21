UNAME_S := $(shell uname -s)

CFLAGS = \
		 -O0	\
		 -g3	\
		 -Wall 		\
		 -Wextra	\

LD_FLAGS = 	\
			-lpthread	\
			-lm		\
			-lglfw	\
			-L/opt/homebrew/lib \
			

INCLUDES =	\
			-I../../mpsLibC/common \
			-I/opt/homebrew/include \

ifeq ($(UNAME_S),Linux)
	LD_FLAGS += -lGL
endif
	
	
MACOS_FLAGS = \
			-framework OpenGL \
			

all: main.c
	gcc $(CFLAGS) $^ $(LD_FLAGS) $(INCLUDES) $(MACOS_FLAGS)
