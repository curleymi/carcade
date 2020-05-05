/*
 *  Michael Curley
 *  snake.h
 */

#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>

#define INITIAL_LENGTH 20
#define BOARD_WIDTH 80
#define BOARD_HEIGHT 40
#define BOARD_SIZE (BOARD_HEIGHT * BOARD_WIDTH)
#define CHAR_ROW_WIDTH (BOARD_WIDTH + 3)
#define CHAR_ROW_COUNT (BOARD_HEIGHT + 3)
#define CHAR_BOARD_SIZE (CHAR_ROW_WIDTH * CHAR_ROW_COUNT)

#define TITLE "CSNAKE"
#define TITLE_LEN 6
#define GAME_OVER_MSG "GAME OVER!"
#define GAME_OVER_MSG_LEN 10

#define SNAKE_GAME_OVER -1

enum e_direction {
    e_direction_up,
    e_direction_down,
    e_direction_left,
    e_direction_right,
};

struct location_t {
    uint8_t row;
    uint8_t col;
};

struct snake_t {
    enum e_direction direction;
    uint32_t length;
    uint32_t offset;
    struct location_t locations[BOARD_SIZE];
};

int make_snake(void);

int print_snake(void);

void snake_over(void);

#endif



