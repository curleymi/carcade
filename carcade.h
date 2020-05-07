/*
 *  Michael Curley
 *  carcade.h
 */

#ifndef CARCADE_H
#define CARCADE_H

#include <unistd.h>

// define this to override any speed to enable extra slow mode
//#define SLOW_MODE
#ifdef SLOW_MODE
// the amount of time between paints in seconds
#define SLOW_SLEEP_SEC                            0.5
#endif

// metric bounds
#define MIN_WIDTH                                 23
#define MAX_WIDTH                                 128
#define MIN_HEIGHT                                6
#define MAX_HEIGHT                                48
#define MIN_SPEED                                 1
#define MAX_SPEED                                 10
#define MIN_FORCE_REFRESH                         0
#define MAX_FORCE_REFRESH                         30

// default metrics
#define WIDTH_ARG                                 "-w"
#define DEFAULT_WIDTH                             40
#define HEIGHT_ARG                                "-h"
#define DEFAULT_HEIGHT                            15
#define SPEED_ARG                                 "-s"
#define DEFAULT_SPEED                             1

// paint defaults
#define TITLE_CHAR_ARG                           "-title"
#define DEFAULT_TITLE_CHAR                       ' '
#define CORNER_CHAR_ARG                          "-corner"
#define DEFAULT_CORNER_CHAR                      '+'
#define HORIZONTAL_CHAR_ARG                       "-hborder"
#define DEFAULT_HORIZONTAL_CHAR                  '-'
#define VERTICAL_CHAR_ARG                        "-vborder"
#define DEFAULT_VERTICAL_CHAR                    '|'
#define CLEAR_CHAR_ARG                           "-board"
#define DEFAULT_CLEAR_CHAR                       ' '

// logic defaults
#define KEEP_SCORE_ARG                           "-freeplay"
#define DEFAULT_KEEP_SCORE                        1 // true
#define DEFAULT_ORKEYS                            0 // false -> single player
#define DEFAULT_SINGLE_KEY                        1 // true
#define DEFAULT_CLEAR_BOARD_BUFFER                1 // true

// the height of the title characters and horizontal border, width of vertical
// border
#define CHAR_TITLE_HEIGHT                         1
#define CHAR_BORDER_WIDTH                         1
#define CHAR_BORDER_HEIGHT                        1

// macro - returns the number of characters in the height of the board
// 2 horizontal borders + title
#define CHAR_BOARD_HEIGHT(HEIGHT) (HEIGHT + CHAR_TITLE_HEIGHT + (CHAR_BORDER_HEIGHT * 2))

// macro - returns the microseconds to sleep according to speed
#define UDELAY(SPEED) (150000 / SPEED)

// the timeout for getting a character, 1/10th second
#define GETCH_TIMEOUT                             1


// the quit and continue string
#define GAME_OVER_MESSAGE                        " GAME OVER "
#define QUIT_MESSAGE_FORMAT                      " PRESS \'%c\' TO QUIT "
#define PLAY_MESSAGE                             " PRESS ANY KEY TO PLAY "
#define EXIT_MESSAGE                             " PRESS ANY KEY TO EXIT "

// the scoreboard format string
#define SCOREBOARD_SCORE                         " SCORE: %d "
#ifdef SLOW_MODE
#define SCOREBOARD_WIDTH_HEIGHT_SPEED            "SIZE: %dx%d  SPEED: SLOW "
#else
#define SCOREBOARD_WIDTH_HEIGHT_SPEED            "SIZE: %dx%d  SPEED: %d "
#endif

// max length of any predefined message
#define MAX_STRLEN                                128

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
    arrow_clear =          ~(1 | 2 | 4 | 8),

    // second player arrow keys
    ascii_up =               16,
    ascii_down =             32,
    ascii_right =            64,
    ascii_left =             128,
    ascii_clear =          ~(16 | 32 | 64 | 128),

    // the quit key
    carcade_quit =           256,
};

// represents the game metrics
struct carcade_t {
    // ----- customizable setup from command line arguments -----
    // generic metrics for every game
    int width;  // chars wide
    int height; // chars high
    int speed;  // paint ticks/second

    // board paint info
    char title_char;      // chars next to title
    char corner_char;     // chars in the corners
    char horizontal_char; // chars on top/bottom
    char vertical_char;   // chars on sides
    char clear_char;      // chars in the middle

    // ----- game specific data, no defualts must be set on initialize -----
    // bool to keep score and if so the current score
    int keep_score;
    int score;

    // bool, indicates if the board is completely cleared after each paint
    int clear_board_buffer;

    // an initial set keystroke for a new game
    enum e_keystroke key;
    // bool to indicate if keystrokes should be ORed together
    // may be useful for multiplayer with multiple keys pressed per paint period
    int ORkeys;
    // bool to indicate if only key direction is accounted for... pressing right
    // then left results in left only
    int single_key;

    // title and gameplay text
    char title[MIN_WIDTH];
    
    // initialize the module - nullable 
    int (*initialize)(struct carcade_t* data);
    // setup/start a new game - nullable
    int (*reset)(void);
    // advance the module specific logic (use paint_char to update board)
    // NON-NULLABLE
    int (*move)(enum e_keystroke next);
    // end the current game (but don't quit) - nullable
    int (*over)(void);
    // stop/quit the game - nullable
    void (*stop)(void);
};

// prints data about the c arcade
void print_carcade_help(void);

// sets the default or overwritten data
void set_data(struct carcade_t* data, int argc, char** argv);

// starts the arcade, allocates resources
int start_carcade(struct carcade_t* data);

// initializes a new game
int new_game(void);

// sets a random location with the set minimum bounds
void random_location_bound(struct location_t* loc, int row, int col);

// sets a random location
void random_location(struct location_t* loc);

// clears the current keystroke value, reset logic
void clear_keystroke(void);

// paints a single character on the board
void paint_char(struct location_t* loc, char c);

// returns the painted character at the location
char painted_char(struct location_t* loc);

// adds the given text on the line in the center of the board
void paint_center_text(int line, const char* str);

// paints the current board
int paint(void);

// stops a running game
int game_over(void);

// clears resources
void stop_carcade(void);

#endif

