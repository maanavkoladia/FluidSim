CFLAGS = \
		 -O0	\
		 -g3	\
		 -Wall 		\
		 -Wextra	\

LD_FLAGS = 	\
			-lpthread	\
			-lm		\
			-lglfw	\
			-lGL

INCLUDES =	\
			-I../../mpsLibC/common/

all: main.c
	gcc $(CFLAGS) $^ $(LD_FLAGS) $(INCLUDES)
