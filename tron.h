/*
 *  Michael Curley
 *  tron.h
 */

#ifndef TRON_H
#define TRON_H

#include "carcade.h"

// the tron game identifier and custom args
#define TRON_ARG "tron"

// the player representation
#define TRON_P1_ARG "-tron-p1"
#define TRON_DEFAULT_P1_CHAR '1'
#define TRON_P2_ARG "-tron-p2"
#define TRON_DEFAULT_P2_CHAR '2'
#define TRON_VERTICAL_ARG "-vtrail"
#define TRON_DEFAULT_VERTICAL_CHAR '|'
#define TRON_HORIZONTAL_ARG "-htrail"
#define TRON_DEFAULT_HORIZONTAL_CHAR '-'

#define TRON_P1_WIN_MESSAGE " P1 WINS! "
#define TRON_P2_WIN_MESSAGE " P2 WINS! "
#define TRON_MAX_MESSAGE_LEN 16

// title string
#define TRON_TITLE " TRON "

// prints the info specific to the tron game
void print_tron_help(void);

// initializes the tron game
int new_tron(struct carcade_t* data, int argc, char** argv);

#endif

