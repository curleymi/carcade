#
#	Michael Curley
#	Makefile
#

all:
	gcc -o csnake \
		-lpthread \
		-lncurses \
		snake.h snake.c \
		main.c

clean:
	rm -rf csnake

