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
static int Flag_Kill_Thread;
static int Flag_Paint_Count;

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
static int set_title(void) {
    int col = 0;
    int title_len = strlen(Data->title);
    // title in the middle
    int title_start = (Data->width / 2) - (title_len / 2);
    // board has edges so add the leading one
    mvaddch(0, col++, Data->title_char);
    // fill in title characters until the title string
    for (int i = 0; i < Data->width; i++) {
        if (i == title_start) {
            mvaddstr(0, col, Data->title);
            col += title_len;
            i += title_len - 1;
        }
        else {
            mvaddch(0, col++, Data->title_char);
        }
    }
    // add the trailing edge
    mvaddch(0, col, Data->title_char);
    return 1;
}

// adds the horizontal border to the given board, returns the next row
static int append_horizontal_border(int line) {
    int col = 0;
    // add the left corner
    mvaddch(line, col++, Data->corner_char);
    // fill in the horizontal border
    for (int i = 0; i < Data->width; i++) {
        mvaddch(line, col++, Data->horizontal_char);
    }
    // add the right corner
    mvaddch(line, col, Data->corner_char);
    return line + 1;
}

// initializes the board
static void initialize_board(void) {
    // clear the whole screen
    clear();
    // set the title
    int col;
    int line = set_title();
    // append the border below the title
    line = append_horizontal_border(line);
    // append the start/end vertical borders for each row and clear filler
    for (int i = 0; i < Data->height; i++) {
        col = 0;
        mvaddch(line, col++, Data->vertical_char);
        for (int j = 0; j < Data->width; j++) {
            mvaddch(line, col++, Data->clear_char);
        }
        mvaddch(line++, col, Data->vertical_char);
    }
    // append the border below the board
    append_horizontal_border(line);
}

// updates Next for the given key
static inline void key_pressed(enum e_keystroke key, enum e_keystroke clear) {
    if (Data->ORkeys) {
        if (Data->single_key) {
            Next = (Next & clear) | key;
        }
        else {
            Next |= key;
        }
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
            Flag_Paint_Count = Data->speed;
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
    int first_line = CHAR_TITLE_HEIGHT + CHAR_BORDER_HEIGHT;
    // go through each row filling in the designated clear chars
    for (int row = 0; row < Data->height; row++) {
        for (int col = 0; col < Data->width; col++) {
            mvaddch(first_line + row, CHAR_BORDER_WIDTH + col, Data->clear_char);
        }
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
    // update the scoreboard
    mvaddstr(CHAR_BOARD_HEIGHT(Data->height), 0, left);
    // refresh curses window
    refresh();
    doupdate();
    if (Flag_Paint_Count++ >= Data->speed) {
        endwin();
        initscr();
        refresh();
        doupdate();
        Flag_Paint_Count = 0;
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
            TITLE_CHAR_ARG    "\t\tchar - the title style\n\t"
            CORNER_CHAR_ARG   "\t\tchar - the corner style\n\t"
            HORIZONTAL_CHAR_ARG "\tchar - the horizontal border style\n\t"
            VERTICAL_CHAR_ARG   "\tchar - the vertical border style\n\t"
            CLEAR_CHAR_ARG    "\t\tchar - the board fill style\n\n",
            MIN_WIDTH, MAX_WIDTH, MIN_HEIGHT, MAX_HEIGHT,
            MIN_SPEED, MAX_SPEED);
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
    data->ORkeys = DEFAULT_ORKEYS;
    data->single_key = DEFAULT_SINGLE_KEY;
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
            if (!strcmp(argv[i], HEIGHT_ARG)) {
                data->height = atoi(argv[++i]);
            }
            if (!strcmp(argv[i], SPEED_ARG)) {
                data->speed = atoi(argv[++i]);
            }
            if (!strcmp(argv[i], TITLE_CHAR_ARG)) {
                data->title_char = *argv[++i];
            }
            if (!strcmp(argv[i], CORNER_CHAR_ARG)) {
                data->corner_char = *argv[++i];
            }
            if (!strcmp(argv[i], HORIZONTAL_CHAR_ARG)) {
                data->horizontal_char = *argv[++i];
            }
            if (!strcmp(argv[i], VERTICAL_CHAR_ARG)) {
                data->vertical_char = *argv[++i];
            }
            if (!strcmp(argv[i], CLEAR_CHAR_ARG)) {
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
            Data->speed < MIN_SPEED || Data->speed > MAX_SPEED) {
        printf("error: something went wrong with specified metrics\n");
        return CARCADE_GAME_QUIT;
    }
    if (pthread_create(&Key_Thread, 0, get_keys, 0)) {
        printf("error: could not monitor user input\n");
        return CARCADE_GAME_QUIT;
    }
    
    // seed random
    srand(time(0) ^ getpid());
    
    // setup curses screen
    initscr();
    noecho();
    curs_set(0);
    
    // initialize the game specific data
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
    Next = Data->key;
    Flag_Quit = 0;
    Flag_Running = 1;
    Flag_Paint_Count = 0;
    clear_board_contents();
    // invoke reset if non-null
    if (Data->reset) {
        (*Data->reset)();
    }
    // paint the board after resetting
    paint_current_board();
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
    if (loc->row >= 0 && loc->row < Data->height &&
            loc->col >= 0 && loc->col < Data->width) {
        mvaddch(CHAR_TITLE_HEIGHT + CHAR_BORDER_HEIGHT + loc->row,
                CHAR_BORDER_WIDTH + loc->col, c);
    }
}

// returns the painted character at the location
char painted_char(struct location_t* loc) {
    return A_CHARTEXT & mvinch(CHAR_TITLE_HEIGHT + CHAR_BORDER_HEIGHT + loc->row,
            CHAR_BORDER_WIDTH + loc->col);
}

// adds the given text on the line in the center of the board
void paint_center_text(int line, const char* str) {
    // only paint if text fits
    int len = strlen(str);
    if (len <= Data->width) {
        mvaddstr(CHAR_TITLE_HEIGHT + CHAR_BORDER_HEIGHT + line,
                CHAR_BORDER_WIDTH + (Data->width / 2) - (len / 2), str);
    }
}

// paints the current board
int paint(void) {
    if (Flag_Quit || Flag_Kill_Thread) {
        return CARCADE_GAME_QUIT;
    }
    // clear the board if specified
    if (Data->clear_board_buffer) {
        clear_board_contents();
    }
    // make the move, move can never be null
    int ret = (*Data->move)(next_key());
    // if the result s not a quit, print the board and wait the delay
    if (ret != CARCADE_GAME_QUIT) {
        paint_current_board();
#ifdef SLOW_MODE
        usleep(1000000 * SLOW_SLEEP_SEC);
#else
        usleep(UDELAY(Data->speed));
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
    int line = (Data->height / 2) - 1;
    // indicate the game is no longer running
    Flag_Running = 0;
    // fill in the quit buffer with the special character
    sprintf(quit_buf, QUIT_MESSAGE_FORMAT, CARCADE_QUIT_CHAR);
    // invoke the optional game over function
    if (Data->over && !(*Data->over)()) {
        line++;
    }
    else {
        paint_center_text(line++, GAME_OVER_MESSAGE);
    }
    // paint the messages on three separate lines
    paint_center_text(line++, quit_buf);
    paint_center_text(line, PLAY_MESSAGE);
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
    doupdate();
    curs_set(1);
    endwin();
}



// ----- end of file -----------------------------------------------------------





