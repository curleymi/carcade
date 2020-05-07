/*
 *  Michael Curley
 *  chopper.c
 */


#include "carcade.h"
#include "chopper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// ----- static globals --------------------------------------------------------


// the chopper
static struct chopper_t {
    char chopper_char;
    char ob_char;
    struct location_t position;
    int offset;
    int ob_freq;
    int orig_ob_freq;
    int level_freq;
    int count;
    int peak_width;
    int orig_peak_width;
    int level;
    int orig_speed;
    time_t last_ob;
    time_t last_level;
    int edge_obs[MAX_WIDTH];
    int middle_obs[MAX_WIDTH];
} chopper;

// the game data
static struct carcade_t* Data;



// ----- static functions ------------------------------------------------------


// resets the chopper game
static int chopper_reset(void) {
    // reset position and obstacles
    chopper.position.row = Data->height / 2;
    chopper.position.col = Data->width / 5;
    chopper.offset = 0;
    chopper.ob_freq = chopper.orig_ob_freq;
    chopper.last_ob = time(0);
    chopper.last_level = time(0);
    chopper.level = Data->height / 3;
    chopper.peak_width = chopper.orig_peak_width;
    for (int i = 0; i < Data->width; i++) {
        chopper.edge_obs[i] = -1;
        chopper.middle_obs[i] = -1;
    }
    // paint the start
    paint_char(&chopper.position, chopper.chopper_char);
    Data->speed = chopper.orig_speed;
    Data->key = 0;
    Data->score = 0;
    clear_keystroke();
    return 0;
}

// moves the chopper, returns if result ends game
static inline int process_position(enum e_keystroke next) {
    if (next & (ascii_up | arrow_up)) {
        if (chopper.position.row == 0) {
            return CARCADE_GAME_OVER;
        }
        chopper.position.row--;
    }
    else if (next & (ascii_down | arrow_down)) {
        if (++chopper.position.row >= Data->height) {
            return CARCADE_GAME_OVER;
        }
    }
    return painted_char(&chopper.position) != Data->clear_char ?
        CARCADE_GAME_OVER : 0;
}

// returns a bool if the metric has been surpassed based on current time
static inline int inc_metric(time_t start, int freq) {
    return time(0) - start >= freq;
}

// moves the chopper and the obstacles
static int chopper_move(enum e_keystroke next) {
    int ob_height;
    int ob_pos = -1;
    struct location_t loc;
    // if its a quit key do nothing
    if (next & carcade_quit) {
        return CARCADE_GAME_QUIT;
    }
    // check to increment level
    if (inc_metric(chopper.last_level, chopper.level_freq)) {
        chopper.position.row = Data->height / 2;
        chopper.edge_obs[(chopper.offset + Data->width - 1) % Data->width] = -1;
        chopper.last_level = time(0);
        chopper.last_ob = time(0);
        chopper.count = 0;
        if (chopper.ob_freq > 1) {
            chopper.ob_freq--;
        }
        else if (chopper.peak_width > 1) {
            chopper.peak_width--;
        }
        else if (Data->speed < MAX_SPEED) {
            Data->speed++;
        }
    }
    // same level, continue normal logic
    //else {
        // advance all obstacles
        ob_height = chopper.edge_obs[(chopper.offset + Data->width - 1) % Data->width];
        // add new obstacle
        if (ob_height < 0) {
            if (chopper.edge_obs[chopper.offset] < 0) {
                if (Data->height - chopper.level > 5) {
                    chopper.level++;
                }
                ob_height = rand() % (chopper.level + 1);
                chopper.count = 0;
                Data->score++;
            }
        }
        // change height of previous obstacle by 1
        else if (++chopper.count >= chopper.peak_width) {
            chopper.count = 0;
            switch (rand() % 3) {
                case 0:
                    if (ob_height > 0) {
                        ob_height--;
                    }
                    break;
                case 2:
                    if (ob_height < chopper.level) {
                        ob_height++;
                    }
                    break;
            }
        }
        // check to add a new middle obstacle
        if (inc_metric(chopper.last_ob, chopper.ob_freq)) {
            ob_pos = rand() % (Data->height - chopper.level);
            chopper.last_ob = time(0);
        }
        // increment the obstacle locations and add them in
        chopper.offset = (chopper.offset + 1) % Data->width;
        chopper.edge_obs[(chopper.offset + Data->width - 1) % Data->width] = ob_height;
        chopper.middle_obs[(chopper.offset + Data->width - 1) % Data->width] = ob_pos;
        // paint here
        for (int col = 0; col < Data->width; col++) {
            loc.col = col;
            ob_height = chopper.edge_obs[(col + chopper.offset) % Data->width];
            ob_pos = chopper.middle_obs[(col + chopper.offset) % Data->width];
            if (ob_height >= 0) {
                for (int row = 0; row < chopper.level; row++) {
                    loc.row = row < ob_height ? row : Data->height - row + ob_height - 1;
                    paint_char(&loc, chopper.ob_char);
                }
            }
            if (ob_height >= 0 && ob_pos >= 0) {
                loc.row = ob_height + ob_pos;
                paint_char(&loc, chopper.ob_char);
            }
        }
    //}
    // move the chopper
    if (process_position(next) == CARCADE_GAME_OVER) {
        return CARCADE_GAME_OVER;
    }
    // paint the chopper
    paint_char(&chopper.position, chopper.chopper_char);
    clear_keystroke();
    return 0;
}



// ----- chopper.h -------------------------------------------------------------


// prints the data specific to chopper
void print_chopper_help(void) {
    printf(CHOPPER_ARG "\n\t"
            "additional arguments for" CHOPPER_TITLE "\n\t"
            //TRON_P1_ARG           "\tchar - the player1 bike style\n\t"
            //TRON_P2_ARG           "\tchar - the player2 bike style\n\t"
            //TRON_VERTICAL_ARG   "\t\tchar - the bike trail style moving vertically\n\t"
            //TRON_HORIZONTAL_ARG "\t\tchar - the bike trail style moving horizontally\n\n");
            "\n");

}

// sets up the data for a new chopper game
int new_chopper(struct carcade_t* data, int argc, char** argv) {
    Data = data;
    // TODO defaults
    chopper.chopper_char = '>';
    chopper.ob_char = '?';
    chopper.orig_ob_freq = 3;
    chopper.level_freq = 10;
    chopper.orig_peak_width = 5;
    chopper.orig_speed = Data->speed;
    // parse out custom arguments
    if (!chopper.chopper_char || !chopper.ob_char ||
            chopper.chopper_char == chopper.ob_char ||
            chopper.ob_char == Data->clear_char ||
            Data->clear_char == chopper.chopper_char) {
        printf("error: something went wrong with the chopper arguments\n");
        return CARCADE_GAME_QUIT;
    }
    // set the title and function pointer data
    int len = strlen(CHOPPER_TITLE);
    memcpy(Data->title, CHOPPER_TITLE, len);
    Data->title[len] = '\0';
    Data->reset = chopper_reset;
    Data->move = chopper_move;
    return 0;
}



// ----- end of file -----------------------------------------------------------





