/*
 *  Michael Curley
 *  main.c
 */


#include "snake.h"
#include <unistd.h>



// ----- preprocessor definitions ----------------------------------------------


// the frequency at which to drive the game/move the snake in microseconds
#define PRINT_FREQUENCY_US 85000



// ----- main ------------------------------------------------------------------


// main entry - expects no arguments for the time
int main(int argc, char** argv) {
    // init the game, on fail do nothing
    if (make_snake() != SNAKE_GAME_OVER) {
        // continue printing to drive the game until the game is over
        while (print_snake() != SNAKE_GAME_OVER) {
            usleep(PRINT_FREQUENCY_US);
        }
        // print message and clean up
        snake_over();
    }
    return 0;
}



// ----- end of file -----------------------------------------------------------





