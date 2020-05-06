/*
 *  Michael Curley
 *  snake.h
 */

#ifndef SNAKE_H
#define SNAKE_H

#include "carcade.h"

// the snake game identifier
#define SNAKE_COMMAND "snake"

// the snakes representation
#define SNAKE_DEFAULT_HEAD_CHAR 'o'
#define SNAKE_DEFAULT_BODY_CHAR '.'
#define SNAKE_DEFAULT_FOOD_CHAR '@'

// title and game over strings
#define SNAKE_TITLE " SNAKE "

// initializes the snake game
int new_snake(struct carcade_t* data, int argc, char** argv);

#endif

