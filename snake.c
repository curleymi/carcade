/*
 *  Michael Curley
 *  snake.c
 */


#include "carcade.h"
#include "snake.h"
#include <string.h>


// ----- static globals --------------------------------------------------------


// the snake itself
static struct snake_t {
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
                ? SNAKE_HEAD_CHAR : SNAKE_BODY_CHAR);
    }
    // assign a random food spot anywhere where the snake is not right now
    random_location_bound(&snake.food_loc, 1, snake.length);
    paint_char(&snake.food_loc, SNAKE_FOOD_CHAR);
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
        for (int i = 1; i < snake.length; i++) {
            // if the head hits the body the game is over
            loc = &snake.locations[(i + snake.offset) % snake.area];
            if (loc->row == head.row && loc->col == head.col) {
                return CARCADE_GAME_OVER;
            }
        }
        // overwrite the current head
        paint_char(snake_head(), SNAKE_BODY_CHAR);
        // if head eats food, increase the length/score and put down new food
        if (head.row == snake.food_loc.row && head.col == snake.food_loc.col) {
            snake.length++;
            Data->score++;
            random_location(&snake.food_loc);
            // paint the new food, old was overwritten
            paint_char(&snake.food_loc, SNAKE_FOOD_CHAR);
        }
        else {
            // increase offset and erease the tail
            paint_char(&snake.locations[snake.offset], Data->clear_char);
            snake.offset = (snake.offset + 1) % snake.area;
        }
        // move the head
        loc = snake_head();
        loc->row = head.row;
        loc->col = head.col;
        // paint the head
        paint_char(&head, SNAKE_HEAD_CHAR);
        Data->key = next;
    }
    return 0;
}



// ----- snake.h ---------------------------------------------------------------


// sets up the data for a new snake game
void new_snake(struct carcade_t* data, int argc, char** argv) {
    Data = data;
    int len = strlen(SNAKE_TITLE);
    // TODO parse out custom arguments
    // set the title and function pointer data
    memcpy(Data->title, SNAKE_TITLE, len);
    Data->clear_board_buffer = 0; // snake remains mostly similar between paints
    Data->ORkeys = 0; // single player
    Data->title[len] = '\0';
    Data->reset = snake_reset;
    Data->move = snake_move;
}



// ----- end of file -----------------------------------------------------------





