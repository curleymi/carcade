


all:
	gcc -o main \
		-lpthread \
		-lncurses \
		snake.h snake.c \
		main.c

clean:
	rm -rf main
