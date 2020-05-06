/*
 *  Michael Curley
 *  snake.h
 */

#ifndef SNAKE_H
#define SNAKE_H

#include "carcade.h"

// the snakes representation
#define SNAKE_HEAD_CHAR 'o'
#define SNAKE_BODY_CHAR '.'
#define SNAKE_FOOD_CHAR '@'

// title and game over strings
#define SNAKE_TITLE " SNAKE "

// initializes the snake game
void new_snake(struct carcade_t* data, int argc, char** argv);

#endif

