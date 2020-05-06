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
static int Flag_Kill_Thread;

// the ticks since the last refresh
static unsigned int Count_Since_Refresh;

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

// updates Next for the given key
static inline void key_pressed(enum e_keystroke key, enum e_keystroke clear) {
    if (Data->ORkeys) {
        Next = (Next & clear) | key;
    }
    else {
        Next = key;
    }
}

// returns if an arrow key was processed
static inline int handle_arrow(char* ch) {
    if ((*ch = getch()) == ARROW_ESCAPE_CHAR) {
        if ((*ch = getch()) == ARROW_IGNORE_CHAR) {
            switch ((*ch = getch())) {
                case ARROW_UP_CHAR:
                    key_pressed(arrow_up, arrow_clear);
                    return 1;
                case ARROW_DOWN_CHAR:
                    key_pressed(arrow_down, arrow_clear);
                    return 1;
                case ARROW_RIGHT_CHAR:
                    key_pressed(arrow_right, arrow_clear);
                    return 1;
                case ARROW_LEFT_CHAR:
                    key_pressed(arrow_left, arrow_clear);
                    return 1;
            }
        }
    }
    return 0;
}

// handles an ascii keystroke
static inline void handle_ascii(char ch) {
    switch (ch) {
        case ASCII_UP_CHAR:
            key_pressed(ascii_up, ascii_clear);
            break;
        case ASCII_DOWN_CHAR:
            key_pressed(ascii_down, ascii_clear);
            break;
        case ASCII_RIGHT_CHAR:
            key_pressed(ascii_right, ascii_clear);
            break;
        case ASCII_LEFT_CHAR:
            key_pressed(ascii_left, ascii_clear);
            break;
        case CARCADE_REFRESH_CHAR:
            Flag_Refresh = 1;
            break;
        case CARCADE_QUIT_CHAR:
            Flag_Quit = 1;
            break;
    }
}

// user input thread handler
static void* get_keys(void* arg) {
    char ch;
    do {
        // only try to read characters if running
        if (Flag_Running) {
            // timeout in case Flag_Quit is set externally
            halfdelay(GETCH_TIMEOUT);
            if (!handle_arrow(&ch)) {
                handle_ascii(ch);
            }
        }
    } while (!Flag_Kill_Thread);
    // set the quit, bypass OR logic
    Next = carcade_quit;
    return NULL;
}

// returns the next keystroke(s)
static inline enum e_keystroke next_key(void) {
    return Flag_Quit || Flag_Kill_Thread ? carcade_quit : Next;
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

// returns the pointer to the character at the location
static inline char* board_char(struct location_t* loc) {
    return Data->board + Data->skip_board_chars +
        (Data->line_width * loc->row) +
        loc->col + CHAR_BORDER_WIDTH;

}

// refreshes the entire curses window
static inline void force_refresh_curses(void) {
    clear();
    endwin();
    initscr();
    noecho();
    clear();
    Count_Since_Refresh = 0;
    Flag_Refresh = 0;
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
    if (Data->keep_score) {
        sprintf(left, SCOREBOARD_SCORE, Data->score);
    }
    else {
        *left = '\0';
    }
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
    if (Flag_Refresh ||
            (Data->force_refresh && Count_Since_Refresh++ > FORCE_REFRESH_COUNT(Data->force_refresh, Data->speed))) {
        force_refresh_curses();
    }
    // add the board and the scoreboard
    mvaddnstr(0, 0, Data->board, Data->board_size);
    mvaddstr(Data->board_lines, 0, left);
    // refresh curses window and clear the local board buffer
    refresh();
    if (Data->clear_board_buffer) {
        clear_board_contents();
    }
}

// gets the first character and clears the input buffer if many keys are clicked
static inline char user_input(void) {
    // wait for any user input
    cbreak();
    char ch = getch();
    // clear the buffer
    do {
        halfdelay(GETCH_TIMEOUT);
    } while (getch() != ERR);
    return ch;
}



// ----- carcade.h -------------------------------------------------------------


// prints data about the c arcade
void print_carcade_help(void) {
    printf("\t"
            WIDTH_ARG         "\t\tint  - the game width between %d and %d\n\t"
            HEIGHT_ARG        "\t\tint  - the game height between %d and %d\n\t"
            SPEED_ARG         "\t\tint  - the game speed between %d and %d\n\t"
            KEEP_SCORE_ARG      "\t     - disable score keeping\n\t"
            FORCE_REFRESH_ARG "\t\tint  - force a new render of the game every %d-%d seconds\n\t"
                              "\t\t       higher values may result in choppy/degraded performance\n\t"
                              "\t\t         NOTE: if omitted the game may have stray characters\n\t"
                              "\t\t           (especially at higher speeds) to force a manual\n\t"
                              "\t\t           refresh render press \'%c\' during gameplay\n\t"
            TITLE_CHAR_ARG    "\t\tchar - the title style\n\t"
            CORNER_CHAR_ARG     "\tchar - the corner style\n\t"
            HORIZONTAL_CHAR_ARG "\tchar - the horizontal border style\n\t"
            VERTICAL_CHAR_ARG   "\tchar - the vertical border style\n\t"
            CLEAR_CHAR_ARG    "\t\tchar - the board fill style\n\n",
            MIN_WIDTH, MAX_WIDTH, MIN_HEIGHT, MAX_HEIGHT,
            MIN_SPEED, MAX_SPEED, MIN_FORCE_REFRESH, MAX_FORCE_REFRESH,
            CARCADE_REFRESH_CHAR);
}

// sets the default data
void set_data(struct carcade_t* data, int argc, char** argv) {
    // set all default values
    data->width = DEFAULT_WIDTH;
    data->height = DEFAULT_HEIGHT;
    data->speed = DEFAULT_SPEED;
    data->keep_score = DEFAULT_KEEP_SCORE;
    data->score = 0;
    data->title_char = DEFAULT_TITLE_CHAR;
    data->corner_char = DEFAULT_CORNER_CHAR;
    data->horizontal_char = DEFAULT_HORIZONTAL_CHAR;
    data->vertical_char = DEFAULT_VERTICAL_CHAR;
    data->clear_char = DEFAULT_CLEAR_CHAR;
    data->force_refresh = DEFAULT_FORCE_REFRESH;
    data->ORkeys = DEFAULT_ORKEYS;
    data->title[0] = '\0'; 
    data->initialize = NULL;
    data->reset = NULL;
    data->move = NULL;
    data->over = NULL;
    data->stop = NULL;
    // parse out specials
    for (int i = 0; i < argc; i++) {
        // argument pairs
        if (i < argc - 1) {
            if (!strcmp(argv[i], WIDTH_ARG)) {
                data->width = atoi(argv[++i]);
            }
            else if (!strcmp(argv[i], HEIGHT_ARG)) {
                data->height = atoi(argv[++i]);
            }
            else if (!strcmp(argv[i], SPEED_ARG)) {
                data->speed = atoi(argv[++i]);
            }
            else if (!strcmp(argv[i], FORCE_REFRESH_ARG)) {
                data->force_refresh = atoi(argv[++i]);
            }
            else if (!strcmp(argv[i], TITLE_CHAR_ARG)) {
                data->title_char = *argv[++i];
            }
            else if (!strcmp(argv[i], CORNER_CHAR_ARG)) {
                data->corner_char = *argv[++i];
            }
            else if (!strcmp(argv[i], HORIZONTAL_CHAR_ARG)) {
                data->horizontal_char = *argv[++i];
            }
            else if (!strcmp(argv[i], VERTICAL_CHAR_ARG)) {
                data->vertical_char = *argv[++i];
            }
            else if (!strcmp(argv[i], CLEAR_CHAR_ARG)) {
                data->clear_char = *argv[++i];
            }
        }
        // single arguments
        if (!strcmp(argv[i], KEEP_SCORE_ARG)) {
            data->keep_score = 0;
        }
    }
}

// starts the arcade. allocates resources
int start_carcade(struct carcade_t* data) {
    // ensure normal terminal
    system("stty sane");

    // indicate not running
    Flag_Running = 0;
    Flag_Quit = 0;
    Flag_Kill_Thread = 0;

    // set and verify data
    Data = data;
    if (!Data->move) {
        printf("error: no move function specified - report\n");
        return CARCADE_GAME_QUIT;
    }
    if (!Data->title_char || !Data->corner_char ||
            !Data->horizontal_char || !Data->vertical_char || !Data->clear_char ||
            Data->width < MIN_WIDTH || Data->width > MAX_WIDTH ||
            Data->height < MIN_HEIGHT || Data->height > MAX_HEIGHT ||
            Data->speed < MIN_SPEED || Data->speed > MAX_SPEED ||
            Data->force_refresh < MIN_FORCE_REFRESH ||
            Data->force_refresh > MAX_FORCE_REFRESH) {
        printf("error: something went wrong with specified metrics\n");
        return CARCADE_GAME_QUIT;
    }
    if (pthread_create(&Key_Thread, 0, get_keys, 0)) {
        printf("error: could not monitor user input\n");
        return CARCADE_GAME_QUIT;
    }
    
    // seed random
    srand(time(0) ^ getpid());
    
    // set the new data
    Data->board_lines = CHAR_BOARD_HEIGHT(Data->height);
    Data->line_width = CHAR_BOARD_WIDTH(Data->width);
    Data->board_size = CHAR_BOARD_SIZE(Data->width, Data->height);
    Data->skip_board_chars = SKIP_BOARD_CHARS(Data->width);
    Data->delay = UDELAY(Data->speed);
    
    // setup curses screen
    initscr();
    noecho();
    
    // initialize the board and the game specific data
    initialize_board();
    return Data->initialize ? (*Data->initialize)(Data) : 0;
}

// initializes a new game
int new_game(void) {
    if (Flag_Kill_Thread) {
        return CARCADE_GAME_QUIT;
    }
    // reset score, can overwrite later if needed
    Data->score = 0;
    clear_board_contents();
    // invoke reset if non-null
    if (Data->reset) {
        (*Data->reset)();
    }
    // paint the board after resetting
    paint_current_board();
    // set the local next key and indicate to thread the game is now running
    Next = Data->key;
    Flag_Quit = 0;
    Flag_Running = 1;
    Flag_Refresh = 0;
    Count_Since_Refresh = 0;
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

// clears the current keystroke value, reset logic
void clear_keystroke(void) {
    Next = 0;
}

// paints a single character on the board
void paint_char(struct location_t* loc, char c) {
    *board_char(loc) = c;
}

// returns the painted character at the location
char painted_char(struct location_t* loc) {
    return *board_char(loc);
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
    if (Flag_Quit || Flag_Kill_Thread) {
        return CARCADE_GAME_QUIT;
    }
    // make the move, move can never be null
    int ret = (*Data->move)(next_key());
    // if the result s not a quit, print the board and wait the delay
    if (ret != CARCADE_GAME_QUIT) {
        paint_current_board();
#ifdef SLOW_MODE
        usleep(1000000 * SLOW_SLEEP_SEC);
#else
        usleep(Data->delay);
#endif
    }
    return ret;
}

// stops a running game
int game_over(void) {
    if (Flag_Kill_Thread) {
        return CARCADE_GAME_QUIT;
    }
    char quit_buf[MAX_STRLEN];
    int line = (Data->height / 2) - 2;
    // indicate the game is no longer running
    Flag_Running = 0;
    // fill in the quit buffer with the special character
    sprintf(quit_buf, QUIT_MESSAGE_FORMAT, CARCADE_QUIT_CHAR);
    // clear the board before adding to it
    clear_board_contents();
    // invoke the optional game over function
    if (Data->over) {
        (*Data->over)();
    }
    // paint the three messages on three separate lines
    paint_center_text(line, GAME_OVER_MESSAGE);
    paint_center_text(line + 1, quit_buf);
    paint_center_text(line + 2, PLAY_MESSAGE);
    paint_current_board();
    // wait for the user input, if quit then stop and return quit
    if (user_input() == CARCADE_QUIT_CHAR) {
        Flag_Quit = 1;
        return CARCADE_GAME_QUIT;
    }
    else {
        Flag_Quit = 0;
    }
    return 0;
}

// clears the board and any other set up
void stop_carcade(void) {
    // indicate no longer running and also stopped
    Flag_Running = 0;
    Flag_Quit = 1;
    Flag_Kill_Thread = 1;
    // wait for thread to join up
    pthread_join(Key_Thread, 0);
    // clear the board before adding to it
    clear_board_contents();
    // invoke the stop function if non-null
    if (Data->stop) {
        (*Data->stop)();
    }
    // paint the exit message
    paint_center_text((Data->height / 2) - 1, EXIT_MESSAGE);
    paint_current_board();
    user_input();
    // clear the screen and end the curses window
    clear();
    refresh();
    endwin();
}



// ----- end of file -----------------------------------------------------------





