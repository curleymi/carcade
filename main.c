/*
 *  Michael Curley
 *  main.c
 */


#include "carcade.h"
#include "chopper.h"
#include "snake.h"
#include "tron.h"
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sigcaught = 0;

static void sighand(int sig) {
    sigcaught = 1;
}

static void print_help(void) {
   printf("help \n");
   print_carcade_help();
   printf("\nto play any of the following games specify its name as the first argument\n\n");
   print_chopper_help();
   print_snake_help();
   print_tron_help();
}

static int initialize_game(struct carcade_t* data, int argc, char** argv) {
   if (argc > 1) {
       if (!strcmp(argv[1], CHOPPER_ARG)) {
           return new_chopper(data, argc, argv);
       }
       if (!strcmp(argv[1], SNAKE_ARG)) {
           return new_snake(data, argc, argv);
       }
       if (!strcmp(argv[1], TRON_ARG)) {
           return new_tron(data, argc, argv);
       }
   }
   print_help();
   return CARCADE_GAME_QUIT;
}

// main entry
int main(int argc, char** argv) {
    int ret;
    struct carcade_t data;
    set_data(&data, argc, argv);
    // handle signal interrupt
    if (signal(SIGINT, sighand) == SIG_ERR) {
        // print error
        return -1;
    }
    // if init failed do nothing
    if (initialize_game(&data, argc, argv) != CARCADE_GAME_QUIT &&
            start_carcade(&data) != CARCADE_GAME_QUIT) {
        // play new games until it is quit or a signal interrupt
        while (!sigcaught && (ret = game_over()) != CARCADE_GAME_QUIT) {
            // create new game
            ret = new_game();
            // continue to play until game over or signal interrupt
            while (!sigcaught && ret != CARCADE_GAME_OVER) {
                // if the user quits give them an opportunity to play again
                if ((ret = paint()) == CARCADE_GAME_QUIT) {
                    ret = CARCADE_GAME_OVER;
                }
            }
        }
        stop_carcade();
    }
    return 0;
}



// ----- end of file -----------------------------------------------------------





