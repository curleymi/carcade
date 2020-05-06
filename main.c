/*
 *  Michael Curley
 *  main.c
 */


#include "carcade.h"
#include "snake.h"
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>

static int sigcaught = 0;

static void sighand(int sig) {
    sigcaught = 1;
    stop_carcade();
}

// main entry
int main(int argc, char** argv) {
    int ret;
    struct carcade_t data;
    default_data(&data);
    if (signal(SIGINT, sighand) == SIG_ERR) {
        return -1;
    }
    // determine game type here
    new_snake(&data, argc, argv);
    ret = start_carcade(&data);
    while (!sigcaught && ret != CARCADE_GAME_QUIT &&
            (ret = game_over()) != CARCADE_GAME_QUIT) {
        ret = new_game();
        while (!sigcaught && ret != CARCADE_GAME_OVER && ret != CARCADE_GAME_QUIT) {
            ret = paint();
        }
    }
    if (!sigcaught) {
        stop_carcade();
    }
    return 0;
}



// ----- end of file -----------------------------------------------------------





