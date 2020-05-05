/*
 *  Michael Curley
 *  snake.c
 */


#include "snake.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <ncurses.h>
#include <string.h>



// ----- static globals --------------------------------------------------------


// the board to print
static char board[CHAR_BOARD_SIZE];

// the thread that handles user input
static pthread_t input_thread;

// the new direction of the snake
static enum e_direction new_direction;

// the snake structure itself
static struct snake_t snake;

// bool indicates if game is over
static int game_over;



// ----- static local functions ------------------------------------------------


// adds the title to the buffer
static char* append_screen_title(void) {
    char* screen = board;
    *(screen++) = ' ';
    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (i == (BOARD_WIDTH / 2) - (TITLE_LEN / 2)) {
            memcpy(screen, TITLE, TITLE_LEN);
            screen += TITLE_LEN;
            i += TITLE_LEN - 1;
        }
        else {
            *(screen++) = ' ';
        }
    }
    *(screen++) = ' ';
    *(screen++) = '\n';
    return screen;
}

// adds the horizontal border
static char* append_horizontal(char* screen, char edge, char fill) {
    *(screen++) = edge;
    for (int i = 0; i < BOARD_WIDTH; i++) {
        *(screen++) = fill;
    }
    *(screen++) = edge;
    *(screen++) = '\n';
    return screen;
}

// sets up the screen buffer
static void setup_screen(void) {
    char* screen = append_screen_title();
    screen = append_horizontal(screen, '+', '-');
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        screen = append_horizontal(screen, '|', ' ');
    }
    screen = append_horizontal(screen, '+', '-');
}

// continually loops until the user input is exit
static void* handle_input(void* arg) {
    char ch;
    do {
        halfdelay(1);
        if ((ch = getch()) == '\033') {
            if ((ch = getch()) == '[') {
                switch ((ch = getch())) {
                    case 'A':
                        new_direction = e_direction_up;
                        break;
                    case 'B':
                        new_direction = e_direction_down;
                        break;
                    case 'C':
                        new_direction = e_direction_right;
                        break;
                    case 'D':
                        new_direction = e_direction_left;
                        break;
                }
            }
        }
    } while (ch != 'q' && game_over != SNAKE_GAME_OVER);
    game_over = SNAKE_GAME_OVER;
    return (void*)0;
}

// gets the head of the snake
static inline struct location_t* snake_head(void) {
    return &snake.locations[(snake.offset + snake.length - 1) % BOARD_SIZE];
}

// gets the next position of the head
static inline void next_location(struct location_t* head,
                                 enum e_direction direction,
                                 uint8_t* new_row,
                                 uint8_t* new_col) {
    switch (direction) {
        case e_direction_up:
            *new_row = (head->row + BOARD_HEIGHT - 1) % BOARD_HEIGHT;
            *new_col = head->col;
            break;
        case e_direction_down:
            *new_row = (head->row + 1) % BOARD_HEIGHT;
            *new_col = head->col;
            break;
        case e_direction_right:
            *new_row = head->row;
            *new_col = (head->col + 1) % BOARD_WIDTH;
            break;
        case e_direction_left:
        default:
            *new_row = head->row;
            *new_col = (head->col + BOARD_WIDTH - 1) % BOARD_WIDTH;
            break;
    }
}

// moves the snake in the given direction
static inline int move_snake(void) {
    uint8_t head_row;
    uint8_t head_col;
    struct location_t* loc;
    enum e_direction next_direction = new_direction;
    if (game_over == SNAKE_GAME_OVER) {
        return game_over;
    }
    if ((snake.direction == e_direction_up && next_direction == e_direction_down) ||
            (snake.direction == e_direction_down && next_direction == e_direction_up) ||
            (snake.direction == e_direction_right && next_direction == e_direction_left) ||
            (snake.direction == e_direction_left && next_direction == e_direction_right)) {
        next_direction = snake.direction;
    }
    loc = snake_head();
    next_location(loc, next_direction, &head_row, &head_col);
    for (int i = 1; i < snake.length; i++) {
        loc = &snake.locations[(i + snake.offset) % BOARD_SIZE];
        if (loc->row == head_row && loc->col == head_col) {
            return (game_over = SNAKE_GAME_OVER);
        }
    }
    snake.offset = (snake.offset + 1) % BOARD_SIZE;
    loc = snake_head();
    loc->row = head_row;
    loc->col = head_col;
    snake.direction = next_direction;
    return 0;
}

// clears the board string of all snake identifiers
static inline void clear_screen_buf(void) {
    struct location_t* loc;
    for (int i = 0; i < snake.length; i++) {
        loc = &snake.locations[(i + snake.offset) % BOARD_SIZE];
        board[(CHAR_ROW_WIDTH * (loc->row + 2)) + loc->col + 1] = ' ';
    }
}

// prints the current screen
static inline void print_screen_buf(void) {
    char buf[256];
    sprintf(buf, "SCORE: %d\n", snake.length);
    clear();
    addnstr(board, CHAR_BOARD_SIZE);
    addstr(buf);
    refresh();
}



// ----- snake.h ---------------------------------------------------------------


// makes a new game
int make_snake(void) {
    if (pthread_create(&input_thread, 0, handle_input, 0)) {
        return SNAKE_GAME_OVER;
    }
    initscr();
    noecho();
    setup_screen();
    game_over = 0;
    new_direction = e_direction_right;
    snake.direction = new_direction;
    snake.length = INITIAL_LENGTH;
    snake.offset = 0;
    for (int i = 0; i < INITIAL_LENGTH; i++) {
        snake.locations[i].row = 0;
        snake.locations[i].col = i;
    }
    return 0;
}

// prints the output of the game advancing the snake by 1 location
int print_snake(void) {
    char c;
    struct location_t* loc;
    struct location_t* head;
    int move = move_snake();
    if (move != SNAKE_GAME_OVER) {
        head = snake_head();
        for (int i = 0; i < snake.length; i++) {
            loc = &snake.locations[(i + snake.offset) % BOARD_SIZE];
            board[(CHAR_ROW_WIDTH * (loc->row + 2)) + loc->col + 1] =
                loc->row == head->row && loc->col == head->col ? 'X' : '0';
        }
        print_screen_buf();
        clear_screen_buf();
    }
    return move;
}

// cleans up snake resources
void snake_over(void) {
    pthread_join(input_thread, 0);
    memcpy(&board[(CHAR_ROW_WIDTH * ((BOARD_HEIGHT / 2) + 2)) + (BOARD_WIDTH / 2) + 1
        - (GAME_OVER_MSG_LEN / 2)], GAME_OVER_MSG, GAME_OVER_MSG_LEN);
    print_screen_buf();
    cbreak();
    getch();
    endwin();
}



// ----- end of file -----------------------------------------------------------





