/* Group: 09
   Members: PARINKARN Sasina   (EID: sparinkar2, ID: 59016540)
            LE Vinh Thanh Linh (EID: vtlle2,     ID: 59257310)
            BONGONI Revan      (EID: rbongoni2,  ID: 59036838)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char job_id[32];
    int arrival_time;
    int burst_time;
    int priority;
} Job;

/*
Purpose: Load jobs from a Q3 input file.
Args: input_path - path to jobs file.
Return: 0 on success, non-zero on error.
*/
static int load_jobs(const char *input_path) {
    (void)input_path;
    /* TODO: Implement parsing and storage of Q3 jobs. */
    return 0;
}

/*
Purpose: Program entry point for Q3 scheduling simulation.
Args: argc - number of command-line arguments.
      argv - command-line argument values.
Return: 0 on success, non-zero on usage or runtime error.
*/
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./scheduler <input_file>\n");
        return 1;
    }

    if (load_jobs(argv[1]) != 0) {
        return 1;
    }

    fprintf(stderr, "TODO: implement Q3 scheduling algorithms in problem3.c\n");
    return 0;
}
