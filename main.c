/*
 *  Michael Curley
 *  main.c
 */


#include "snake.h"

#include <unistd.h>

int main(int argc, char** argv) {
    struct snake_t snake;
    if (make_snake() == SNAKE_GAME_OVER) {
        return 1;
    }
    while (print_snake() != SNAKE_GAME_OVER) {
        usleep(85000);
    }
    snake_over();
    return 0;
}
