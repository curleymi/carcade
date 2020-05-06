/*
 *  Michael Curley
 *  carcade.c
 */


#include "carcade.h"
#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// ----- static globals --------------------------------------------------------


// flag indicates if the game is running and if game has been quit
static int Flag_Running;
static int Flag_Quit;
static int Flag_Refresh;

// the thread-shared next keystroke to process
// note:
//  - not protecting with a lock since we want this to go as fast as possible,
//    in hopes that the game is rendered quick enough that missing a keystroke
//    over a delay period does not matter
static enum e_keystroke Next;

// the data specific to the game set up
static struct carcade_t* Data;

// the keystroke processing thread
static pthread_t Key_Thread;



// ----- static functions ------------------------------------------------------


// adds the title to the data board, returns the pointer to the next row
static char* set_title(void) {
    char* brd = Data->board;
    int title_len = strlen(Data->title);
    // title in the middle
    int title_start = (Data->width / 2) - (title_len / 2);
    // board has edges so add the leading one
    *(brd++) = Data->title_char;
    // fill in title characters until the title string
    for (int i = 0; i < Data->width; i++) {
        if (i == title_start) {
            memcpy(brd, Data->title, title_len);
            brd += title_len;
            i += title_len - 1;
        }
        else {
            *(brd++) = Data->title_char;
        }
    }
    // add the trailing edge and newline
    *(brd++) = Data->title_char;
    *(brd++) = '\n';
    return brd;
}

// adds the horizontal border to the given board, returns the next row
static char* append_horizontal_border(char* brd) {
    // add the left corner
    *(brd++) = Data->corner_char;
    // fill in the horizontal border
    for (int i = 0; i < Data->width; i++) {
        *(brd++) = Data->horizontal_char;
    }
    // add the right corner and newline
    *(brd++) = Data->corner_char;
    *(brd++) = '\n';
    return brd;
}

// initializes the board
static void initialize_board(void) {
    // set the title
    char* brd = set_title();
    // append the border below the title
    brd = append_horizontal_border(brd);
    // eppend the start and end vertical borders for each row
    for (int i = 0; i < Data->height; i++) {
        *(brd++) = Data->vertical_char;
        for (int j = 0; j < Data->width; j++) {
            *(brd++) = Data->clear_char;
        }
        *(brd++) = Data->vertical_char;
        *(brd++) = '\n';
    }
    // append the border below the board
    append_horizontal_border(brd);
}

// user input thread handler
static void* get_keys(void* arg) {
    int set;
    char ch;
    do {
        // only try to read characters if running
        if (Flag_Running) {
            set = 0;
            // timeout in case Flag_Quit is set externally
            halfdelay(GETCH_TIMEOUT);
            // if the sequence of chars leads to an arrow, indicate Next has
            // been set
            if ((ch = getch()) == ARROW_ESCAPE_CHAR) {
                if ((ch = getch()) == ARROW_IGNORE_CHAR) {
                    switch ((ch = getch())) {
                        case ARROW_UP_CHAR:
                            Next = arrow_up;
                            set = 1;
                            break;
                        case ARROW_DOWN_CHAR:
                            Next = arrow_down;
                            set = 1;
                            break;
                        case ARROW_RIGHT_CHAR:
                            Next = arrow_right;
                            set = 1;
                            break;
                        case ARROW_LEFT_CHAR:
                            Next = arrow_left;
                            set = 1;
                            break;
                    }
                }
            }
            // if not an arrow char, process a regular character
            if (!set) {
                switch (ch) {
                    case ASCII_UP_CHAR:
                        Next = ascii_up;
                        break;
                    case ASCII_DOWN_CHAR:
                        Next = ascii_down;
                        break;
                    case ASCII_RIGHT_CHAR:
                        Next = ascii_right;
                        break;
                    case ASCII_LEFT_CHAR:
                        Next = ascii_left;
                        break;
                    case CARCADE_REFRESH_CHAR:
                        Flag_Refresh = 1;
                        break;
                    case CARCADE_QUIT_CHAR:
                        Flag_Quit = 1;
                        break;
                }
            }
        }
    } while (!Flag_Quit);
    Next = carcade_quit;
    return NULL;
}

// returns the next keystroke(s)
static inline enum e_keystroke next_key(void) {
    return Flag_Quit ? carcade_quit : Next;
}

// clears the active gameboard
static inline void clear_board_contents(void) {
    // skip over initial characters
    char* brd = Data->board + Data->skip_board_chars + CHAR_BORDER_WIDTH;
    // go through each row filling in the designated clear chars
    for (int row = 0; row < Data->height; row++) {
        for (int col = 0; col < Data->width; col++) {
            *(brd++) = Data->clear_char;
        }
        // skip over the edges and newline
        brd += (CHAR_BORDER_WIDTH * 2) + 1;
    }
}

// paints the current contents of the board to the console
static inline void paint_current_board(void) {
    char* buf;
    char left[MAX_STRLEN];
    char right[MAX_STRLEN];
    int left_len;
    int right_len;
    int filler;
    // fill in the scoreboard labels
    sprintf(left, SCOREBOARD_SCORE, Data->score);
    sprintf(right, SCOREBOARD_WIDTH_HEIGHT_SPEED, Data->width, Data->height, Data->speed);
    left_len = strlen(left);
    right_len = strlen(right);
    // append the right label to the left separated by as many spaces as
    // possible to give the appearance of aligned text
    buf = left + left_len;
    for (int i = 0; i < Data->width + (2 * CHAR_BORDER_WIDTH) - left_len - right_len; i++) {
        *(buf++) = ' ';
    }
    // copy over the right label, add newline and null terminate
    memcpy(buf, right, right_len);
    buf += right_len;
    *(buf++) = '\n';
    *buf = '\0';
    // refresh if needed
    if (Flag_Refresh) {
        clear();
        endwin();
        initscr();
        noecho();
        clear();
        Flag_Refresh = 0;
    }
    // add the board and the scoreboard
    mvaddnstr(0, 0, Data->board, Data->board_size);
    mvaddstr(Data->board_lines, 0, left);
    // refresh curses window and clear the local board buffer
    refresh();
    clear_board_contents();
}



// ----- carcade.h -------------------------------------------------------------


// sets the default data
void default_data(struct carcade_t* data) {
    // set all default values
    data->width = DEFAULT_WIDTH;
    data->height = DEFAULT_HEIGHT;
    data->speed = DEFAULT_SPEED;
    data->score = 0;
    data->title_char = DEFAULT_TITLE_CHAR;
    data->corner_char = DEFAULT_CORNER_CHAR;
    data->horizontal_char = DEFAULT_HORIZONTAL_CHAR;
    data->vertical_char = DEFAULT_VERTICAL_CHAR;
    data->clear_char = DEFAULT_CLEAR_CHAR;
    data->title[0] = '\0'; 
    data->initialize = NULL;
    data->reset = NULL;
    data->move = NULL;
    data->over = NULL;
    data->stop = NULL;

}

// starts the arcade. allocates resources
int start_carcade(struct carcade_t* data) {
    // ensure normal terminal
    system("stty sane");

    // indicate not running
    Flag_Running = 0;
    Flag_Quit = 0;
    Flag_Refresh = 0;

    // set and verify data
    Data = data;
    if (!Data->move ||
            Data->width < MIN_WIDTH || Data->width > MAX_WIDTH ||
            Data->height < MIN_HEIGHT || Data->height > MAX_HEIGHT ||
            Data->speed < MIN_SPEED || Data->speed > MAX_SPEED ||
            pthread_create(&Key_Thread, 0, get_keys, 0)) {
        return CARCADE_GAME_QUIT;
    }
    // setup curses screen
    initscr();
    noecho();
    
    // seed random
    srand(time(0) ^ getpid());
    
    // set the new data
    Data->board_lines = CHAR_BOARD_HEIGHT(Data->height);
    Data->line_width = CHAR_BOARD_WIDTH(Data->width);
    Data->board_size = CHAR_BOARD_SIZE(Data->width, Data->height);
    Data->skip_board_chars = SKIP_BOARD_CHARS(Data->width);
    Data->delay = UDELAY(Data->speed);
    
    // initialize the board and the game specific data
    initialize_board();
    return Data->initialize ? (*Data->initialize)(Data) : 0;
}

// initializes a new game
int new_game(void) {
    if (Flag_Quit) {
        return CARCADE_GAME_QUIT;
    }
    // reset score, can overwrite later if needed
    Data->score = 0;
    // invoke reset if non-null
    if (Data->reset) {
        (*Data->reset)();
    }
    // paint the board after resetting
    paint_current_board();
    // set the local next key and indicate to thread the game is now running
    Next = Data->key;
    Flag_Running = 1;
    return 0;
}

// sets a random location with the set minimum bounds
void random_location_bound(struct location_t* loc, int row, int col) {
    loc->row = (rand() % (Data->height - row)) + row;
    loc->col = (rand() % (Data->width - col)) + col;
}

// sets a random location
void random_location(struct location_t* loc) {
    random_location_bound(loc, 0, 0);
}

// paints a single character on the board
void paint_char(struct location_t* loc, char c) {
    Data->board[Data->skip_board_chars + (Data->line_width * loc->row) + loc->col + CHAR_BORDER_WIDTH] = c;
}

// adds the given text on the line in the center of the board
void paint_center_text(int line, const char* str) {
    // only paint if text fits
    int len = strlen(str);
    if (len <= Data->width) {
        // get the center location at the given line
        struct location_t loc = { line, (Data->width / 2) - (len / 2) };
        // add all the characters
        for (int i = 0; i < len; i++) {
            paint_char(&loc, str[i]);
            loc.col++;
        }
    }
}

// paints the current board
int paint(void) {
    if (Flag_Quit) {
        return CARCADE_GAME_QUIT;
    }
    // make the move, move can never be null
    int ret = (*Data->move)(next_key());
    // if the result s not a quit, print the board and wait the delay
    if (ret != CARCADE_GAME_QUIT) {
        paint_current_board();
        usleep(Data->delay);
    }
    return ret;
}

// stops a running game
int game_over(void) {
    if (Flag_Quit) {
        return CARCADE_GAME_QUIT;
    }
    char quit_buf[MAX_STRLEN];
    int line = (Data->height / 2) - 2;
    // indicate the game is no longer running
    Flag_Running = 0;
    // fill in the quit buffer with the special character
    sprintf(quit_buf, QUIT_MESSAGE_FORMAT, CARCADE_QUIT_CHAR);
    // paint the three messages on three separate lines
    paint_center_text(line, GAME_OVER_MESSAGE);
    paint_center_text(line + 1, quit_buf);
    paint_center_text(line + 2, PLAY_MESSAGE);
    paint_current_board();
    // wait for the user input, if quit then stop and return quit
    cbreak();
    if (getch() == CARCADE_QUIT_CHAR) {
        Flag_Quit = 1;
        return CARCADE_GAME_QUIT;
    }
    return 0;
}

// clears the board and any other set up
void stop_carcade(void) {
    // indicate no longer running and also stopped
    Flag_Running = 0;
    Flag_Quit = 1;
    // wait for thread to join up
    pthread_join(Key_Thread, 0);
    // invoke the stop function if non-null
    if (Data->stop) {
        (*Data->stop)();
    }
    // paint the exit message
    paint_center_text((Data->height / 2) - 1, EXIT_MESSAGE);
    paint_current_board();
    // wait for any user input
    cbreak();
    getch();
    // clear the buffer
    do {
        halfdelay(GETCH_TIMEOUT);
    } while (getch() != ERR);
    // clear the screen and end the curses window
    clear();
    refresh();
    endwin();
}



// ----- end of file -----------------------------------------------------------





