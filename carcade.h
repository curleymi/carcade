/*
 *  Michael Curley
 *  carcade.h
 */

#ifndef CARCADE_H
#define CARCADE_H

#include <unistd.h>

// metric bounds
#define MIN_WIDTH                                 23
#define MAX_WIDTH                                 128
#define MIN_HEIGHT                                6
#define MAX_HEIGHT                                48
#define MIN_SPEED                                 1
#define MAX_SPEED                                 10

// default metrics
#define DEFAULT_WIDTH                             40
#define DEFAULT_HEIGHT                            15
#define DEFAULT_SPEED                             1

// the height of the title characters and horizontal border, width of vertical
// border
#define CHAR_TITLE_HEIGHT                         1
#define CHAR_BORDER_WIDTH                         1
#define CHAR_BORDER_HEIGHT                        1

// macro - returns the number of characters in the width of the board
// 2 vertical chars + newline
#define CHAR_BOARD_WIDTH(WIDTH) (WIDTH + (CHAR_BORDER_WIDTH * 2) + 1)

// macro - returns the number of characters in the height of the board
// 2 horizontal borders + title
#define CHAR_BOARD_HEIGHT(HEIGHT) (HEIGHT + CHAR_TITLE_HEIGHT + (CHAR_BORDER_HEIGHT * 2))

// macro - returns the total number of characters in the board
#define CHAR_BOARD_SIZE(WIDTH, HEIGHT) (CHAR_BOARD_WIDTH(WIDTH) * CHAR_BOARD_HEIGHT(HEIGHT))

// macro - returns the number of characters to skip before the first valid row
#define SKIP_BOARD_CHARS(WIDTH) (CHAR_BOARD_WIDTH(WIDTH) * (CHAR_TITLE_HEIGHT + CHAR_BORDER_HEIGHT))

// macro - returns the microseconds to sleep according to speed
#define UDELAY(SPEED) (150000 / SPEED)

// the timeout for getting a character, 1/10th second
#define GETCH_TIMEOUT                             1

// the quit and continue string
#define GAME_OVER_MESSAGE                        " GAME OVER "
#define QUIT_MESSAGE_FORMAT                      " PRESS %c TO QUIT "
#define PLAY_MESSAGE                             " PRESS ANY KEY TO PLAY "
#define EXIT_MESSAGE                             " PRESS ANY KEY TO EXIT "

// the scoreboard format string
#define SCOREBOARD_SCORE                         " SCORE: %d "
#define SCOREBOARD_WIDTH_HEIGHT_SPEED            "SIZE: %dx%d  SPEED: %d "

// max length of any predefined message
#define MAX_STRLEN                                128

// paint defaults
#define DEFAULT_TITLE_CHAR                       ' '
#define DEFAULT_CORNER_CHAR                      '+'
#define DEFAULT_HORIZONTAL_CHAR                  '-'
#define DEFAULT_VERTICAL_CHAR                    '|'
#define DEFAULT_CLEAR_CHAR                       ' '

// key identifiers
#define ARROW_ESCAPE_CHAR                        '\033'
#define ARROW_IGNORE_CHAR                        '['
#define ARROW_UP_CHAR                            'A'
#define ARROW_DOWN_CHAR                          'B'
#define ARROW_RIGHT_CHAR                         'C'
#define ARROW_LEFT_CHAR                          'D'
#define ASCII_UP_CHAR                            'w'
#define ASCII_DOWN_CHAR                          's'
#define ASCII_RIGHT_CHAR                         'd'
#define ASCII_LEFT_CHAR                          'a'
#define CARCADE_REFRESH_CHAR                     'r'
#define CARCADE_QUIT_CHAR                        'q'

// return codes
#define CARCADE_GAME_OVER                        -1
#define CARCADE_GAME_QUIT                        -2

// a location in a grid, size pending max width/height
struct location_t {
    unsigned char row;
    unsigned char col;
};

// the different supported keystrokes, values may be ORed together
enum e_keystroke {
    // arrow keys
    arrow_up =               1,
    arrow_down =             2,
    arrow_right =            4,
    arrow_left =             8,

    // second player arrow keys
    ascii_up =               16,
    ascii_down =             32,
    ascii_right =            64,
    ascii_left =             128,

    // the quit key
    carcade_quit =           256,
};

// represents the game metrics
struct carcade_t {
    // ----- customizable setup from command line arguments -----
    // generic metrics for every game
    int width;    // chars wide
    int height;  // chars high
    int speed;    // paint ticks/second
    int score;                // current score

    // board paint info
    char title_char;
    char corner_char;
    char horizontal_char;
    char vertical_char;
    char clear_char;
    char board[CHAR_BOARD_SIZE(MAX_WIDTH, MAX_HEIGHT) + CHAR_BOARD_WIDTH(MAX_WIDTH)];

    // ----- game specific data, no defualts must be set on initialize -----
    // the height and width of the board in lines
    int board_lines;
    int line_width;

    // the total size of the board (not necessarily the max as above)
    int board_size;

    // the amount of chars to skip before the board
    int skip_board_chars;

    // the delay time in microsecions
    useconds_t delay;

    // maintain state of previous keystroke
    enum e_keystroke key;

    // title and gameplay text
    char title[MIN_WIDTH];
    
    // functions for gameplay, must return a carcade constant
    int (*initialize)(struct carcade_t* data);
    int (*reset)(void);
    int (*move)(enum e_keystroke next);
    void (*over)(void);
    void (*stop)(void);
};

// sets the default data
void default_data(struct carcade_t* data);

// starts the arcade, allocates resources
int start_carcade(struct carcade_t* data);

// initializes a new game
int new_game(void);

// sets a random location with the set minimum bounds
void random_location_bound(struct location_t* loc, int row, int col);

// sets a random location
void random_location(struct location_t* loc);

// paints a single character on the board
void paint_char(struct location_t* loc, char c);

// adds the given text on the line in the center of the board
void paint_center_text(int line, const char* str);

// paints the current board
int paint(void);

// stops a running game
int game_over(void);

// clears resources
void stop_carcade(void);

#endif

