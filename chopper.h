/*
 *  Michael Curley
 *  chopper.h
 */

#ifndef CHOPPER_H
#define CHOPPER_H

#include "carcade.h"

// the chopper game identifier and custom args
#define CHOPPER_ARG "chopper"

// title string
#define CHOPPER_TITLE " CHOPPER "

// prints the info specific to the chopper game
void print_chopper_help(void);

// initializes the chopper game
int new_chopper(struct carcade_t* data, int argc, char** argv);

#endif

