/* Group: 09
   Members: PARINKARN Sasina   (EID: sparinkar2, ID: 59016540)
            LE Vinh Thanh Linh (EID: vtlle2,     ID: 59257310)
            BONGONI Revan      (EID: rbongoni2,  ID: 59036838)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "../include/helpers.h"

typedef struct {
    int order_id;
    int raw_value;
} RawPacket;

typedef struct {
    int order_id;
    int encoded_value;
} EncodedPacket;

typedef struct {
    void *data;
    int capacity;
    int head;
    int tail;
    size_t esz;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} BoundedBuffer;

typedef struct {
    int T;
    sem_t *token_sem;
} TokenPool;

typedef struct {
    int remaining;
    int P;
    pthread_mutex_t mutex;
    BoundedBuffer  *buf_B;
} TermState;

typedef struct {
    int quantizer_id;   
    int start_order;    
    int end_order;      
    BoundedBuffer *buf_A;
} QuantizerArg;

typedef struct {
    int encoder_id;    
    int token_type_A;
    int token_type_B;
    BoundedBuffer *buf_A;
    TokenPool *pool;
    TermState *term;
} EncoderArg;

typedef struct {
    int P;    
    BoundedBuffer *buf_B;
} LoggerArg;

void buf_init(BoundedBuffer *buf, int capacity, size_t esz)
{
    buf->data = malloc((size_t)capacity * esz);
    if (buf->data == NULL) {
        fprintf(stderr, "Error: malloc failed for buffer data\n");
        exit(1);
    }
    buf->capacity = capacity;
    buf->head = 0;
    buf->tail = 0;
    buf->esz = esz;
    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty, 0, (unsigned)capacity);
    sem_init(&buf->full,  0, 0);
}

void buf_destroy(BoundedBuffer *buf)
{
    free(buf->data);
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
}

void buf_put(BoundedBuffer *buf, const void *item)
{
    sem_wait(&buf->empty);
    pthread_mutex_lock(&buf->mutex);
    memcpy((char *)buf->data + (size_t)buf->tail * buf->esz, item, buf->esz);
    buf->tail = (buf->tail + 1) % buf->capacity;
    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->full);
}

void buf_get(BoundedBuffer *buf, void *item)
{
    sem_wait(&buf->full);
    pthread_mutex_lock(&buf->mutex);
    memcpy(item, (char *)buf->data + (size_t)buf->head * buf->esz, buf->esz);
    buf->head = (buf->head + 1) % buf->capacity;
    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->empty);
}

void pool_init(TokenPool *pool, int T, const int *counts)
{
    pool->T         = T;
    pool->token_sem = malloc((size_t)T * sizeof(sem_t));
    if (pool->token_sem == NULL) {
    fprintf(stderr, "Error: malloc failed for token semaphore array\n");
    exit(1);
    }
    for (int i = 0; i < T; i++)
    sem_init(&pool->token_sem[i], 0, (unsigned)counts[i]);
}

void pool_destroy(TokenPool *pool)
{
    for (int i = 0; i < pool->T; i++)
        sem_destroy(&pool->token_sem[i]);
    free(pool->token_sem);
}

void acquire_two_tokens(TokenPool *pool, int tA, int tB)
{
    int lo = (tA < tB) ? tA : tB;
    int hi = (tA < tB) ? tB : tA;
    sem_wait(&pool->token_sem[lo]);
    sem_wait(&pool->token_sem[hi]);
}

void release_two_tokens(TokenPool *pool, int tA, int tB)
{
    sem_post(&pool->token_sem[tA]);
    sem_post(&pool->token_sem[tB]);
}

/*
Purpose: Quantizer thread entry for Q1 pipeline.
Args: arg - pointer to QuantizerArg for this thread.
Return: NULL when thread completes.
*/
void *quantizer_thread(void *arg)
{
    QuantizerArg *a = (QuantizerArg *)arg;

    for (int id = a->start_order; id < a->end_order; id++) {
        simulate_work(OP_Q1_QUANTIZE);
        RawPacket pkt;
        pkt.order_id  = id;
        pkt.raw_value = id * 3 + 7;
        buf_put(a->buf_A, &pkt);
    }

    RawPacket sentinel = { .order_id = -1, .raw_value = 0 };
    buf_put(a->buf_A, &sentinel);
    return NULL;
}

/*
Purpose: Encoder thread entry for Q1 pipeline.
Args: arg - pointer to EncoderArg for this thread.
Return: NULL when thread completes.
*/
void *encoder_thread(void *arg)
{
    EncoderArg *a = (EncoderArg *)arg;
    int tA = a->token_type_A;
    int tB = a->token_type_B;

    while (1) {
        RawPacket raw;
        buf_get(a->buf_A, &raw);

        if (raw.order_id == -1) {
            pthread_mutex_lock(&a->term->mutex);
            a->term->remaining--;
            int last = (a->term->remaining == 0);
            pthread_mutex_unlock(&a->term->mutex);

            if (last) {
            EncodedPacket s = { .order_id = -1, .encoded_value = 0 };
                for (int i = 0; i < a->term->P; i++)
                    buf_put(a->term->buf_B, &s);
            }
            break;
        }

        acquire_two_tokens(a->pool, tA, tB);
        simulate_work(OP_Q1_ENCODE);
        int encoded = raw.raw_value * 2 + tA + tB;
        release_two_tokens(a->pool, tA, tB);

        EncodedPacket enc;
        enc.order_id      = raw.order_id;
        enc.encoded_value = encoded;
        buf_put(a->term->buf_B, &enc);
    }

    return NULL;
}

/*
Purpose: Logger thread entry for Q1 pipeline.
Args: arg - optional argument for logger thread.
Return: NULL when thread completes.
*/
void *logger_thread(void *arg)
{
    LoggerArg *a       = (LoggerArg *)arg;
    int sentinels_seen = 0;

    while (sentinels_seen < a->P) {
        EncodedPacket enc;
        buf_get(a->buf_B, &enc);

        if (enc.order_id == -1) {
            sentinels_seen++;
            continue;
        }

        simulate_work(OP_Q1_LOG);
        printf("[Logger] order_id=%d encoded=%d\n",
        enc.order_id, enc.encoded_value);
    }
    return NULL;
}

/*
Purpose: Validate encoder token parameters.
Args: P - number of encoders.
      T - number of token types.
      tA, tB - token type arrays (length P).
Return: 0 on success, non-zero on error. Prints stderr on failure.
*/
static int validate_encoder_tokens(int P, int T, const int *tA, const int *tB)
{
    for (int i = 0; i < P; i++) {
        if (tA[i] == tB[i]) {
            fprintf(stderr, "Error: encoder %d has tA[%d]=%d same as tB[%d]=%d\n",
                i, i, tA[i], i, tB[i]);
            return 1;
        }
        if (tA[i] < 0 || tA[i] >= T) {
            fprintf(stderr, "Error: encoder %d has tA[%d]=%d out of range [0, %d)\n",
                i, i, tA[i], T);
            return 1;
        }
        if (tB[i] < 0 || tB[i] >= T) {
            fprintf(stderr, "Error: encoder %d has tB[%d]=%d out of range [0, %d)\n",
                i, i, tB[i], T);
            return 1;
        }
    }
    return 0;
}

/*
Purpose: Parse command-line arguments for the Q1 pipeline.
Args: argc - number of command-line arguments.
      argv - command-line argument values.
      P, M, N, num_orders, T - output parameters for parsed values.
      counts, tA, tB - output arrays for token configuration.
Return: 0 on success, non-zero on error.
*/
static int parse_args(int argc, char *argv[],
int *P, int *M, int *N, int *num_orders, int *T,
int **counts, int **tA, int **tB)
{
    if (argc < 6) {
        fprintf(stderr,
            "Usage: %s P M N num_orders T cnt_0..cnt_{T-1} "
            "tA_0 tB_0 ... tA_{P-1} tB_{P-1}\n", argv[0]);
        return 1;
    }
    *P = atoi(argv[1]);
    *M = atoi(argv[2]);
    *N = atoi(argv[3]);
    *num_orders = atoi(argv[4]);
    *T = atoi(argv[5]);

    int need = 6 + *T + 2 * (*P);
    if (argc < need) {
        fprintf(stderr, "Need %d arguments, got %d.\n", need, argc);
        return 1;
    }

    *counts = malloc((size_t)(*T) * sizeof(int));
    if (*counts == NULL) {
        fprintf(stderr, "Error: malloc failed for counts array\n");
        exit(1);
    }
    for (int i = 0; i < *T; i++)
        (*counts)[i] = atoi(argv[6 + i]);

    *tA = malloc((size_t)(*P) * sizeof(int));
    if (*tA == NULL) {
        fprintf(stderr, "Error: malloc failed for tA array\n");
        free(*counts);
        exit(1);
    }
    *tB = malloc((size_t)(*P) * sizeof(int));
    if (*tB == NULL) {
        fprintf(stderr, "Error: malloc failed for tB array\n");
        free(*counts);
        free(*tA);
        exit(1);
    }
    for (int i = 0; i < *P; i++) {
        (*tA)[i] = atoi(argv[6 + *T + 2 * i]);
        (*tB)[i] = atoi(argv[6 + *T + 2 * i + 1]);
    }
    return 0;
}

/*
Purpose: Initialize and run the order pipeline with all threads.
         Handles buffer/pool/thread allocation, spawning, joining, and cleanup.
Args: P - number of encoder/quantizer threads.
      M - capacity of buf_A (raw packets).
      N - capacity of buf_B (encoded packets).
      num_orders - total number of orders to process.
      T - number of token types.
      counts - array of token counts (length T).
      tA, tB - token type arrays for each encoder (length P).
Return: 0 on success, non-zero on error.
Note: Does not free counts, tA, tB; caller owns those.
*/
static int run_order_pipeline(int P, int M, int N, int num_orders, int T,
                              const int *counts, const int *tA, const int *tB)
{
    BoundedBuffer buf_A, buf_B;
    buf_init(&buf_A, M, sizeof(RawPacket));
    buf_init(&buf_B, N, sizeof(EncodedPacket));

    TokenPool pool;
    pool_init(&pool, T, counts);

    TermState term;
    term.remaining = P;
    term.P  = P;
    term.buf_B = &buf_B;
    pthread_mutex_init(&term.mutex, NULL);

    pthread_t *qt   = malloc((size_t)P * sizeof(pthread_t));
    if (qt == NULL) {
        fprintf(stderr, "Error: malloc failed for quantizer thread array\n");
        return 1;
    }
    QuantizerArg *qarg = malloc((size_t)P * sizeof(QuantizerArg));
    if (qarg == NULL) {
        fprintf(stderr, "Error: malloc failed for quantizer arguments array\n");
        free(qt);
        return 1;
    }

    int base  = num_orders / P;
    int extra = num_orders % P;
    int start = 0;
    for (int i = 0; i < P; i++) {
        int chunk            = base + (i < extra ? 1 : 0);
        qarg[i].quantizer_id = i;
        qarg[i].start_order = start;
        qarg[i].end_order = start + chunk;
        qarg[i].buf_A = &buf_A;
        int ret = pthread_create(&qt[i], NULL, quantizer_thread, &qarg[i]);
        if (ret != 0) {
            perror("pthread_create (quantizer)");
            return 1;
        }
        start += chunk;
    }

    pthread_t  *et   = malloc((size_t)P * sizeof(pthread_t));
    if (et == NULL) {
        fprintf(stderr, "Error: malloc failed for encoder thread array\n");
        free(qt);
        free(qarg);
        return 1;
    }
    EncoderArg *earg = malloc((size_t)P * sizeof(EncoderArg));
    if (earg == NULL) {
        fprintf(stderr, "Error: malloc failed for encoder arguments array\n");
        free(qt);
        free(qarg);
        free(et);
        return 1;
    }
    for (int i = 0; i < P; i++) {
        earg[i].encoder_id = i;
        earg[i].token_type_A = tA[i];
        earg[i].token_type_B = tB[i];
        earg[i].buf_A = &buf_A;
        earg[i].pool = &pool;
        earg[i].term = &term;
        int ret = pthread_create(&et[i], NULL, encoder_thread, &earg[i]);
        if (ret != 0) {
            perror("pthread_create (encoder)");
            free(qt);
            free(qarg);
            free(et);
            free(earg);
            return 1;
        }
    }

    LoggerArg larg = { .P = P, .buf_B = &buf_B };
    pthread_t  lt;
    int ret = pthread_create(&lt, NULL, logger_thread, &larg);
    if (ret != 0) {
        perror("pthread_create (logger)");
        free(qt);
        free(qarg);
        free(et);
        free(earg);
        return 1;
    }

    /* Join threads in order: quantizers, encoders, logger */
    for (int i = 0; i < P; i++) pthread_join(qt[i], NULL);
    for (int i = 0; i < P; i++) pthread_join(et[i], NULL);
    pthread_join(lt, NULL);

    /* Cleanup thread-local resources */
    buf_destroy(&buf_A);
    buf_destroy(&buf_B);
    pool_destroy(&pool);
    pthread_mutex_destroy(&term.mutex);
    free(qt);
    free(qarg);
    free(et);
    free(earg);

    return 0;
}

/*
Purpose: Coordinate parsing, validation, and running the Q1 pipeline.
         Handles main-owned allocations (counts, tA, tB).
Args: argc - number of command-line arguments.
      argv - command-line argument values.
Return: 0 on success, non-zero on error.
*/
int main(int argc, char *argv[])
{
    int  P, M, N, num_orders, T;
    int *counts = NULL, *tA = NULL, *tB = NULL;

    if (parse_args(argc, argv, &P, &M, &N, &num_orders, &T,
                   &counts, &tA, &tB) != 0)
        return 1;

    if (validate_encoder_tokens(P, T, tA, tB) != 0) {
        free(counts);
        free(tA);
        free(tB);
        return 1;
    }

    int ret = run_order_pipeline(P, M, N, num_orders, T, counts, tA, tB);

    free(counts);
    free(tA);
    free(tB);

    return ret;
}

