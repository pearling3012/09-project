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

typedef struct {
    int thread_id;
} ThreadArg;

/*
Purpose: Quantizer thread entry for Q1 pipeline.
Args: arg - pointer to ThreadArg for this thread.
Return: NULL when thread completes.
*/
void *quantizer_thread(void *arg) {
    (void)arg;
    /* TODO: Implement quantizer behavior. */
    return NULL;
}

/*
Purpose: Encoder thread entry for Q1 pipeline.
Args: arg - pointer to ThreadArg for this thread.
Return: NULL when thread completes.
*/
void *encoder_thread(void *arg) {
    (void)arg;
    /* TODO: Implement encoder behavior. */
    return NULL;
}

/*
Purpose: Logger thread entry for Q1 pipeline.
Args: arg - optional argument for logger thread.
Return: NULL when thread completes.
*/
void *logger_thread(void *arg) {
    (void)arg;
    /* TODO: Implement logger behavior. */
    return NULL;
}

/*
Purpose: Parse command-line arguments and run the Q1 pipeline.
Args: argc - number of command-line arguments.
      argv - command-line argument values.
Return: 0 on success, non-zero on error.
*/
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    fprintf(stderr, "TODO: implement Q1 in problem1.c\n");
    return 0;
}
