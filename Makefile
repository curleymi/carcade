#
#	Michael Curley
#	Makefile
#

all:
	gcc -o carcade \
		-lpthread \
		-lncurses \
		carcade.h carcade.c \
		snake.h snake.c \
		main.c

clean:
	rm -rf carcade

