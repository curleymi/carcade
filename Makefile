#
#	Michael Curley
#	Makefile
#

all:
	gcc -o carcade \
		-lpthread \
		-lncurses \
		carcade.h carcade.c \
		chopper.h chopper.c \
		snake.h snake.c \
		tron.h tron.c \
		main.c

clean:
	rm -rf carcade

