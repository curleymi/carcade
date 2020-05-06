/*
 *  Michael Curley
 *  tron.c
 */


#include "carcade.h"
#include "tron.h"
#include <stdio.h>
#include <string.h>


// ----- static globals --------------------------------------------------------


// the tron players
static struct tron_t {
    char player1_char;
    char player2_char;
    char vertical_char;
    char horizontal_char;
    enum e_keystroke player1_dir;
    enum e_keystroke player2_dir;
    struct location_t player1_loc;
    struct location_t player2_loc;
    char over_message[TRON_MAX_MESSAGE_LEN];
} tron;

// the game data
static struct carcade_t* Data;



// ----- static functions ------------------------------------------------------


// sets the next direction based on the current
static inline enum e_keystroke turn_player(enum e_keystroke cur, enum e_keystroke new) {
    if (!new ||
            (cur & (arrow_up | ascii_up)) && (new & (arrow_down | ascii_down)) ||
            (cur & (arrow_down | ascii_down)) && (new & (arrow_up | ascii_up)) ||
            (cur & (arrow_right | ascii_right)) && (new & (arrow_left | ascii_left)) ||
            (cur & (arrow_left | ascii_left)) && (new & (arrow_right | ascii_right))) {
        new = cur;
    }
    return new;
}
// sets the next position of both players based on the key
static inline int advance_player(struct location_t* loc, enum e_keystroke key, struct location_t* new) {
    if (key & (arrow_up | ascii_up)) {
        new->row = loc->row - 1;
        new->col = loc->col;
    }
    else if (key & (arrow_down | ascii_down)) {
        new->row = loc->row + 1;
        new->col = loc->col;
    }
    else if (key & (arrow_right | ascii_right)) {
        new->row = loc->row;
        new->col = loc->col + 1;
    }
    else if (key & (arrow_left | ascii_left)) {
        new->row = loc->row;
        new->col = loc->col - 1;
    }
    // any other key we don't know what to do with so error out
    else {
        return CARCADE_GAME_OVER;
    }
    return new->row < 0 || new->row >= Data->height || new->col < 0 || new->col >= Data->width ||
        painted_char(new) != Data->clear_char ? CARCADE_GAME_OVER : 0; 
}

// paints the line based on the previous keystroke
static inline void paint_line(struct location_t* loc, enum e_keystroke prev, enum e_keystroke new) {
    paint_char(loc, prev & new & (ascii_up | arrow_up | ascii_down | arrow_down)
            ? tron.vertical_char : tron.horizontal_char);
}


// resets the tron game
static int tron_reset(void) {
    // reset the tron data
    int x = Data->width / 10;
    int y = Data->height - 1;
    tron.player1_dir = ascii_up;
    tron.player2_dir = arrow_up;
    tron.player1_loc.row = y;
    tron.player1_loc.col = x;
    tron.player2_loc.row = y;
    tron.player2_loc.col = Data->width - x;
    // paint the start
    paint_char(&tron.player1_loc, tron.player1_char);
    paint_char(&tron.player2_loc, tron.player2_char);
    Data->key = arrow_up | ascii_up;
    *tron.over_message = '\0';
    return 0;
}


// moves the tron players in their respective directions
static int tron_move(enum e_keystroke next) {
    int res1;
    int res2;
    enum e_keystroke p1_new_dir;
    enum e_keystroke p2_new_dir;
    struct location_t p1_new_loc;
    struct location_t p2_new_loc;
    // if its a quit key do nothing
    if (next & carcade_quit) {
        return CARCADE_GAME_QUIT;
    }
    // can't double back, keep going the same direction if the next is
    // immediately backwards
    p1_new_dir = turn_player(tron.player1_dir, next & arrow_clear);
    p2_new_dir = turn_player(tron.player2_dir, next & ascii_clear);
    // make both moves
    res1 = advance_player(&tron.player1_loc, p1_new_dir, &p1_new_loc);
    res2 = advance_player(&tron.player2_loc, p2_new_dir, &p2_new_loc);
    // if both game over or if both result in the same position...
    if ((res1 == CARCADE_GAME_OVER && res2 == CARCADE_GAME_OVER) ||
            p1_new_loc.row == p2_new_loc.row && p1_new_loc.col == p2_new_loc.col) {
        *tron.over_message = '\0';
        return CARCADE_GAME_OVER;
    }
    if (res1 == CARCADE_GAME_OVER) {
        memcpy(tron.over_message, TRON_P2_WIN_MESSAGE, strlen(TRON_P2_WIN_MESSAGE) + 1);
        return CARCADE_GAME_OVER;
    }
    if (res2 == CARCADE_GAME_OVER) {
        memcpy(tron.over_message, TRON_P1_WIN_MESSAGE, strlen(TRON_P1_WIN_MESSAGE) + 1);
        return CARCADE_GAME_OVER;
    }
    // both moves are valid paint over old positions
    paint_line(&tron.player1_loc, tron.player1_dir, p1_new_dir);
    paint_line(&tron.player2_loc, tron.player2_dir, p2_new_dir);
    // overwrite current positions
    tron.player1_dir = p1_new_dir;
    tron.player2_dir = p2_new_dir;
    tron.player1_loc.row = p1_new_loc.row;
    tron.player1_loc.col = p1_new_loc.col;
    tron.player2_loc.row = p2_new_loc.row;
    tron.player2_loc.col = p2_new_loc.col;
    // paint new positions
    paint_char(&tron.player1_loc, tron.player1_char);
    paint_char(&tron.player2_loc, tron.player2_char);
    return 0;
}

// prints the winner if there is one
static int tron_over(void) {
    if (*tron.over_message) {
        paint_center_text((Data->height / 2) - 1, tron.over_message);
        return 0;
    }
    return 1;
}



// ----- tron.h ----------------------------------------------------------------


// prints the data specific to tron
void print_tron_help(void) {
    printf(TRON_ARG "\n\t"
            "additional arguments for" TRON_TITLE "\n\t"
            TRON_P1_ARG           "\tchar - the player1 bike style\n\t"
            TRON_P2_ARG           "\tchar - the player2 bike style\n\t"
            TRON_VERTICAL_ARG   "\t\tchar - the bike trail style moving vertically\n\t"
            TRON_HORIZONTAL_ARG "\t\tchar - the bike trail style moving horizontally\n\n");

}

// sets up the data for a new tron game
int new_tron(struct carcade_t* data, int argc, char** argv) {
    Data = data;
    tron.player1_char = TRON_DEFAULT_P1_CHAR;
    tron.player2_char = TRON_DEFAULT_P2_CHAR;
    tron.vertical_char = TRON_DEFAULT_VERTICAL_CHAR;
    tron.horizontal_char = TRON_DEFAULT_HORIZONTAL_CHAR;
    // parse out custom arguments
    for (int i = 0; i < argc - 1; i++) {
        if (!strcmp(TRON_P1_ARG, argv[i])) {
            tron.player1_char = *argv[++i];
        }
        else if (!strcmp(TRON_P2_ARG, argv[i])) {
            tron.player2_char = *argv[++i];
        }
        else if (!strcmp(TRON_VERTICAL_ARG, argv[i])) {
            tron.vertical_char = *argv[++i];
        }
        else if (!strcmp(TRON_HORIZONTAL_ARG, argv[i])) {
            tron.horizontal_char = *argv[++i];
        }
    }
    if (!tron.player1_char || !tron.player2_char || !tron.vertical_char || !tron.horizontal_char ||
            tron.player1_char == tron.player2_char ||
            tron.vertical_char == Data->clear_char || tron.horizontal_char == Data->clear_char) {
        printf("error: something went wrong with the tron arguments\n");
        return CARCADE_GAME_QUIT;
    }
    // set the title and function pointer data
    int len = strlen(TRON_TITLE);
    memcpy(Data->title, TRON_TITLE, len);
    Data->title[len] = '\0';
    Data->ORkeys = 1;
    Data->clear_board_buffer = 0; // tron remains mostly similar between paints
    Data->keep_score = 0;
    Data->reset = tron_reset;
    Data->move = tron_move;
    Data->over = tron_over;
    return 0;
}



// ----- end of file -----------------------------------------------------------





