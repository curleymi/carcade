/*
 *  Michael Curley
 *  snake.h
 */

#ifndef SNAKE_H
#define SNAKE_H

#include "carcade.h"

// the snake game identifier and custom args
#define SNAKE_ARG "snake"
#define SNAKE_HEAD_ARG "-snake-head"
#define SNAKE_BODY_ARG "-snake-body"
#define SNAKE_FOOD_ARG "-snake-food"

// the snakes representation
#define SNAKE_DEFAULT_HEAD_CHAR 'o'
#define SNAKE_DEFAULT_BODY_CHAR '.'
#define SNAKE_DEFAULT_FOOD_CHAR '@'

// title and game over strings
#define SNAKE_TITLE " SNAKE "

// prints the info specific to the snake game
void print_snake_help(void);

// initializes the snake game
int new_snake(struct carcade_t* data, int argc, char** argv);

#endif

