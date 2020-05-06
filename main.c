/*
 *  Michael Curley
 *  main.c
 */


#include "carcade.h"
#include "snake.h"
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
   printf("No game type specified\n");
}

static int initialize_game(struct carcade_t* data, int argc, char** argv) {
   if (argc > 1) {
       if (!strcmp(argv[1], SNAKE_COMMAND)) {
           return new_snake(data, argc, argv);
       }
   }
   print_help();
   return CARCADE_GAME_QUIT;
}

// main entry
int main(int argc, char** argv) {
    int ret;
    struct carcade_t data;
    default_data(&data);
    // handle signal interrupt
    if (signal(SIGINT, sighand) == SIG_ERR) {
        // print error
        return -1;
    }
    // if init failed do nothing
    if (initialize_game(&data, argc, argv) != CARCADE_GAME_QUIT) {
        // start the c arcade
        ret = start_carcade(&data);
        // play new games until it is quit or a signal interrupt
        while (!sigcaught && ret != CARCADE_GAME_QUIT &&
                (ret = game_over()) != CARCADE_GAME_QUIT) {
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





