/*
 *  Michael Curley
 *  snake.h
 */


#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>



// ----- preprocessor definitions ----------------------------------------------


// the initial length of the snake
#define INITIAL_LENGTH 7

// the board metrics
#define BOARD_WIDTH 40
#define BOARD_HEIGHT 15
#define BOARD_SIZE (BOARD_HEIGHT * BOARD_WIDTH)

// the snakes representation
#define HEAD_CHAR 'o'
#define BODY_CHAR '.'
#define FOOD_CHAR '@'

// the board representation
#define TITLE_CHAR ' '
#define CORNER_CHAR '+'
#define HORIZONTAL_CHAR '-'
#define VERTICAL_CHAR '|'
#define SPACE_CHAR ' '

// quit keystroke
#define QUIT_CHAR 'q'

// title and game over strings
#define TITLE " CSNAKE "
#define TITLE_LEN 8
#define GAME_OVER_MSG " GAME OVER! "
#define GAME_OVER_MSG_LEN 12
#define GAME_INFO_FORMAT_DDDDC "BOARD: %dx%d   SCORE: %d (%d%)\n(use arrow keys to move or press \'%c\' to quit)\n"

// timeout input get every 1/10th of a second
#define INPUT_TIMEOUT_DS 1

// the character width of the board (leading/trailing edge + \n)
#define CHAR_ROW_WIDTH (BOARD_WIDTH + 3)

// the number of rows (title, leading/trailing edge)
#define CHAR_ROW_COUNT (BOARD_HEIGHT + 3)

// the total size of the board string
#define CHAR_BOARD_SIZE (CHAR_ROW_WIDTH * CHAR_ROW_COUNT)

// game over constant
#define SNAKE_GAME_OVER -1



// ----- custom types ----------------------------------------------------------


// one of the directions the snake may be traveling
enum e_direction {
    e_direction_up,
    e_direction_down,
    e_direction_left,
    e_direction_right,
};

// a location of a snake part
struct location_t {
    uint8_t row;
    uint8_t col;
};

// the snake itself
struct snake_t {
    enum e_direction direction; // the current direction it is facing
    uint32_t length; // the current length
    uint32_t offset; // the offset into locations of the tail of the snake
    struct location_t locations[BOARD_SIZE]; // the locations of the snake between
                                             // offset and length
};



// ----- prototypes ------------------------------------------------------------


// initializes the snake game
int make_snake(void);

// prints the board after making the next move, printing drives the game steps
int print_snake(void);

// prints message and cleans up resources when the game is over
void snake_over(void);

#endif


// ----- end of file -----------------------------------------------------------




