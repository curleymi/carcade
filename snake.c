/*
 *  Michael Curley
 *  snake.c
 *
 *  notes:
 *      - requires ncurses compiler flag -lncurses
 *          (apt-get install libncurses5-dev)
 *      - requires pthread compiler flag -lpthread
 */


#include "snake.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>



// ----- static globals --------------------------------------------------------


// the board to print
static char board[CHAR_BOARD_SIZE];

// the thread that handles user input
static pthread_t input_thread;

// the new direction of the snake
static enum e_direction new_direction;

// the snake structure itself
static struct snake_t snake;

// the current location of the food
static struct location_t food_loc;

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
            *(screen++) = TITLE_CHAR;
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
    screen = append_horizontal(screen, CORNER_CHAR, HORIZONTAL_CHAR);
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        screen = append_horizontal(screen, VERTICAL_CHAR, SPACE_CHAR);
    }
    screen = append_horizontal(screen, CORNER_CHAR, HORIZONTAL_CHAR);
}

// continually loops until the user input is to quit
static void* handle_input(void* arg) {
    char ch;
    do {
        halfdelay(INPUT_TIMEOUT_DS);
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
    } while (ch != QUIT_CHAR && game_over != SNAKE_GAME_OVER);
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

// set a new location of the food
static inline void set_new_food(int row, int col) {
    food_loc.row = (rand() % (BOARD_HEIGHT - row)) + row;
    food_loc.col = (rand() % (BOARD_WIDTH - col)) + col;
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
    if (head_row == food_loc.row && head_col == food_loc.col) {
        snake.length++;
        set_new_food(0, 0);
    }
    else {
        snake.offset = (snake.offset + 1) % BOARD_SIZE;
    }
    loc = snake_head();
    loc->row = head_row;
    loc->col = head_col;
    snake.direction = next_direction;
    return 0;
}

// inserts the snake to the screen buffer
static inline void insert_snake_buf(void) {
    struct location_t* loc;
    struct location_t* head = snake_head();
    board[(CHAR_ROW_WIDTH * (food_loc.row + 2)) + food_loc.col + 1] = FOOD_CHAR;
    for (int i = 0; i < snake.length; i++) {
        loc = &snake.locations[(i + snake.offset) % BOARD_SIZE];
        board[(CHAR_ROW_WIDTH * (loc->row + 2)) + loc->col + 1] =
            loc->row == head->row && loc->col == head->col ? HEAD_CHAR : BODY_CHAR;
    }
}

// clears the board string of all snake identifiers
static inline void clear_screen_buf(void) {
    struct location_t* loc;
    for (int i = 0; i < snake.length; i++) {
        loc = &snake.locations[(i + snake.offset) % BOARD_SIZE];
        board[(CHAR_ROW_WIDTH * (loc->row + 2)) + loc->col + 1] = SPACE_CHAR;
    }
}

// prints the current screen
static inline void print_screen_buf(void) {
    char buf[256];
    sprintf(buf, GAME_INFO_FORMAT_DDDDC, BOARD_WIDTH, BOARD_HEIGHT,
            snake.length, (int)((100.0 * (double)snake.length / (double)BOARD_SIZE) + 0.5),
            QUIT_CHAR);
    mvaddnstr(0, 0, board, CHAR_BOARD_SIZE);
    mvaddstr(CHAR_ROW_COUNT, 0, buf);
    refresh();
}



// ----- snake.h ---------------------------------------------------------------


// makes a new game
int make_snake(void) {
    if (BOARD_WIDTH < 10 || BOARD_HEIGHT < 10 ||
            INITIAL_LENGTH >= BOARD_WIDTH ||
            pthread_create(&input_thread, 0, handle_input, 0)) {
        return SNAKE_GAME_OVER;
    }
    srand(time(0) ^ getpid());
    initscr();
    noecho();
    clear();
    refresh();
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
    set_new_food(1, INITIAL_LENGTH);
    return 0;
}

// prints the output of the game advancing the snake by 1 location
int print_snake(void) {
    int move = move_snake();
    if (move != SNAKE_GAME_OVER) {
        board[(CHAR_ROW_WIDTH * (food_loc.row + 2)) + food_loc.col + 1] = FOOD_CHAR;
        insert_snake_buf();
        print_screen_buf();
        clear_screen_buf();
    }
    return move;
}

// cleans up snake resources
void snake_over(void) {
    pthread_join(input_thread, 0);
    insert_snake_buf();
    memcpy(&board[(CHAR_ROW_WIDTH * ((BOARD_HEIGHT / 2) + 2)) + (BOARD_WIDTH / 2) + 1
        - (GAME_OVER_MSG_LEN / 2)], GAME_OVER_MSG, GAME_OVER_MSG_LEN);
    print_screen_buf();
    cbreak();
    getch();
    clear();
    refresh();
    endwin();
}



// ----- end of file -----------------------------------------------------------





