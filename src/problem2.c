/* Group: 09
   Members: PARINKARN Sasina   (EID: sparinkar2, ID: 59016540)
            LE Vinh Thanh Linh (EID: vtlle2,     ID: 59257310)
            BONGONI Revan      (EID: rbongoni2,  ID: 59036838)
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/helpers.h"

/*
Purpose: Parse Q2 input file and run directory concurrency simulation.
Args: input_path - path to the Q2 operation file.
Return: 0 on success, non-zero on error.
*/
static int run_simulation(const char *input_path) {
    (void)input_path;
    /* TODO: Implement Q2 reader/writer priority simulation. */
    return 0;
}

/*
Purpose: Program entry point for Q2.
Args: argc - number of command-line arguments.
      argv - command-line argument values.
Return: 0 on success, non-zero on usage or runtime error.
*/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./warehouse <input_file>\n");
        return 1;
    }

    return run_simulation(argv[1]);
}
