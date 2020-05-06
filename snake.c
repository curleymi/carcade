/*
 *  Michael Curley
 *  snake.c
 */


#include "carcade.h"
#include "snake.h"
#include <stdio.h>
#include <string.h>


// ----- static globals --------------------------------------------------------


// the snake itself
static struct snake_t {
    char head_char;
    char body_char;
    char food_char;
    unsigned int length;
    unsigned int offset;
    unsigned int area;
    struct location_t food_loc;
    struct location_t locations[MAX_WIDTH * MAX_HEIGHT];
} snake;

// the game data
static struct carcade_t* Data;



// ----- static functions ------------------------------------------------------


// gets the head of the snake
static inline struct location_t* snake_head(void) {
    return &snake.locations[(snake.offset + snake.length - 1) % snake.area];
}


// gets the next position of the head
static inline int next_location(struct location_t* head,
                                enum e_keystroke key,
                                struct location_t* new_loc) {
    // ensure wraparound for each key direction, also ensure only positive
    // row/col locations
    if (key & (arrow_up | ascii_up)) {
        new_loc->row = (head->row + Data->height - 1) % Data->height;
        new_loc->col = head->col;
    }
    else if (key & (arrow_down | ascii_down)) {
        new_loc->row = (head->row + 1) % Data->height;
        new_loc->col = head->col;
    }
    else if (key & (arrow_right | ascii_right)) {
        new_loc->row = head->row;
        new_loc->col = (head->col + 1) % Data->width;
    }
    else if (key & (arrow_left | ascii_left)) {
        new_loc->row = head->row;
        new_loc->col = (head->col + Data->width - 1) % Data->width;
    }
    // any other key we don't know what to do with so error out
    else {
        return CARCADE_GAME_OVER;
    }
    return 0;
}


// resets the snake game
static int snake_reset(void) {
    // reset the snake data
    snake.offset = 0;
    snake.length = Data->width / 5;
    snake.area = Data->width * Data->height;
    // reset the locations
    for (int i = 0; i < snake.length; i++) {
        snake.locations[i].row = 0;
        snake.locations[i].col = i;
        paint_char(&snake.locations[i], i == snake.length - 1
                ? snake.head_char : snake.body_char);
    }
    // assign a random food spot anywhere where the snake is not right now
    random_location_bound(&snake.food_loc, 1, snake.length);
    paint_char(&snake.food_loc, snake.food_char);
    // set the starting direction
    Data->key = arrow_right;
    return 0;
}


// moves the snake in the given direction
static int snake_move(enum e_keystroke next) {
    struct location_t head;
    struct location_t* loc;
    // if its a quit key do nothing
    if (next & carcade_quit) {
        return CARCADE_GAME_QUIT;
    }
    // can't double back, keep going the same direction if the next is
    // immediately backwards
    if (!next ||
            (Data->key & (arrow_up | ascii_up)) && (next & (arrow_down | ascii_down)) ||
            (Data->key & (arrow_down | ascii_down)) && (next & (arrow_up | ascii_up)) ||
            (Data->key & (arrow_right | ascii_right)) && (next & (arrow_left | ascii_left)) ||
            (Data->key & (arrow_left | ascii_left)) && (next & (arrow_right | ascii_right))) {
        next = Data->key;
    }
    // make sure the next move does not end the game before continuing
    if (next_location(snake_head(), next, &head) != CARCADE_GAME_OVER) {
        // if the head hits the body the game is over
        if (Data->keep_score && painted_char(&head) == snake.body_char) {
            return CARCADE_GAME_OVER;
        }
        // overwrite the current head
        paint_char(snake_head(), snake.body_char);
        // if head eats food, increase the length/score and put down new food
        if (head.row == snake.food_loc.row && head.col == snake.food_loc.col) {
            snake.length++;
            Data->score++;
            random_location(&snake.food_loc);
            // paint the new food, old was overwritten
            paint_char(&snake.food_loc, snake.food_char);
        }
        else {
            // increase offset and erase the tail if it is not the food
            // random food locations means the food can overlap the body
            loc = &snake.locations[snake.offset];
            if (loc->row != snake.food_loc.row || loc->col != snake.food_loc.col) {
                paint_char(&snake.locations[snake.offset], Data->clear_char);
            }
            snake.offset = (snake.offset + 1) % snake.area;
        }
        // move the head
        loc = snake_head();
        loc->row = head.row;
        loc->col = head.col;
        // paint the head if it is not overlapped by the food
        if (head.row != snake.food_loc.row || head.col != snake.food_loc.col) {
            paint_char(&head, snake.head_char);
        }
        Data->key = next;
    }
    return 0;
}



// ----- snake.h ---------------------------------------------------------------


// prints the data specific to snake
void print_snake_help(void) {
    char body = SNAKE_DEFAULT_BODY_CHAR;
    printf(SNAKE_ARG "\n\t"
            "additional arguments for" SNAKE_TITLE "    %c%c%c%c%c%c%c%c %c\n\t"
            SNAKE_HEAD_ARG "\tchar - the head style\n\t"
            SNAKE_BODY_ARG "\tchar - the body style\n\t"
            SNAKE_FOOD_ARG "\tchar - the food style\n\n",
            body, body, body, body, body, body, body,
            SNAKE_DEFAULT_HEAD_CHAR, SNAKE_DEFAULT_FOOD_CHAR);

}

// sets up the data for a new snake game
int new_snake(struct carcade_t* data, int argc, char** argv) {
    Data = data;
    snake.head_char = SNAKE_DEFAULT_HEAD_CHAR;
    snake.body_char = SNAKE_DEFAULT_BODY_CHAR;
    snake.food_char = SNAKE_DEFAULT_FOOD_CHAR;
    // parse out custom arguments
    for (int i = 0; i < argc - 1; i++) {
        if (!strcmp(SNAKE_HEAD_ARG, argv[i])) {
            snake.head_char = *argv[++i];
        }
        else if (!strcmp(SNAKE_BODY_ARG, argv[i])) {
            snake.body_char = *argv[++i];
        }
        else if (!strcmp(SNAKE_FOOD_ARG, argv[i])) {
            snake.food_char = *argv[++i];
        }
    }
    if (!snake.head_char || !snake.body_char || !snake.food_char ||
            snake.head_char == snake.body_char || snake.head_char == Data->clear_char ||
            snake.body_char == snake.food_char || snake.body_char == Data->clear_char ||
            snake.food_char == snake.head_char || snake.food_char == Data->clear_char) {
        printf("error: something went wrong with the snake arguments\n");
        return CARCADE_GAME_QUIT;
    }
    // set the title and function pointer data
    int len = strlen(SNAKE_TITLE);
    memcpy(Data->title, SNAKE_TITLE, len);
    Data->clear_board_buffer = 0; // snake remains mostly similar between paints
    Data->title[len] = '\0';
    Data->reset = snake_reset;
    Data->move = snake_move;
    return 0;
}



// ----- end of file -----------------------------------------------------------





