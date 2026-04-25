/* Group: 09
   Members: PARINKARN Sasina   (EID: sparinkar2, ID: 59016540)
            LE Vinh Thanh Linh (EID: vtlle2,     ID: 59257310)
            BONGONI Revan      (EID: rbongoni2,  ID: 59036838)
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/helpers.h"

#define MAX_FILENAME_LEN 256
#define MAX_FILE_ENTRIES 1000
#define MAX_OPERATIONS 10000
#define INITIAL_FILE_SIZE 2048

typedef struct {
    char filename[MAX_FILENAME_LEN];
    int size;
} DirectoryEntry;

typedef struct {
    DirectoryEntry entries[MAX_FILE_ENTRIES];
    int num_entries;
} Directory;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cv_readers;
    pthread_cond_t cv_managers;
    pthread_cond_t cv_supervisors;
    
    int active_readers;
    int active_writers;
    int waiting_supervisors;
    int waiting_managers;
} DirLock;

typedef enum {
    OP_READ,
    OP_WRITE
} OpType;

typedef enum {
    ROLE_WORKER,
    ROLE_MANAGER,
    ROLE_SUPERVISOR
} ThreadRole;

typedef struct {
    ThreadRole role;
    int id;
    OpType op;
    char filename[MAX_FILENAME_LEN];
} Operation;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    ThreadRole role;
    int id;
    Operation op;
    Directory *dir;
    DirLock *lock;
} ThreadArgs;

static void directory_init(Directory *dir) {
    dir->num_entries = 0;
}

static int directory_add_or_get(Directory *dir, const char *filename) {
    for (int i = 0; i < dir->num_entries; i++) {
        if (strcmp(dir->entries[i].filename, filename) == 0) return i;
    }
    if (dir->num_entries >= MAX_FILE_ENTRIES) return -1;
    strcpy(dir->entries[dir->num_entries].filename, filename);
    dir->entries[dir->num_entries].size = INITIAL_FILE_SIZE;
    return dir->num_entries++;
}

static int directory_get_size(Directory *dir, const char *filename) {
    int idx = directory_add_or_get(dir, filename);
    if (idx >= 0) return dir->entries[idx].size;
    return INITIAL_FILE_SIZE;
}

static void directory_set_size(Directory *dir, const char *filename, int new_size) {
    int idx = directory_add_or_get(dir, filename);
    if (idx >= 0) dir->entries[idx].size = new_size;
}

static void dirlock_init(DirLock *lock) {
    pthread_mutex_init(&lock->mutex, NULL);
    pthread_cond_init(&lock->cv_readers, NULL);
    pthread_cond_init(&lock->cv_managers, NULL);
    pthread_cond_init(&lock->cv_supervisors, NULL);
    
    lock->active_readers = 0;
    lock->active_writers = 0;
    lock->waiting_supervisors = 0;
    lock->waiting_managers = 0;
}

static void dirlock_destroy(DirLock *lock) {
    pthread_mutex_destroy(&lock->mutex);
    pthread_cond_destroy(&lock->cv_readers);
    pthread_cond_destroy(&lock->cv_managers);
    pthread_cond_destroy(&lock->cv_supervisors);
}

static int dirlock_read_acquire(DirLock *lock) {
    pthread_mutex_lock(&lock->mutex);
    
    int was_blocked = 0;
    while (lock->waiting_supervisors > 0 || lock->active_writers > 0) {
        was_blocked = 1;
        pthread_cond_wait(&lock->cv_readers, &lock->mutex);
    }
    
    if (was_blocked) {
        lock->active_readers++;
        pthread_mutex_unlock(&lock->mutex);
        return -1;
    }
    
    int concurrent = (lock->active_readers > 0) ? 1 : 0;
    lock->active_readers++;
    pthread_mutex_unlock(&lock->mutex);
    return concurrent;
}

static void dirlock_read_release(DirLock *lock) {
    pthread_mutex_lock(&lock->mutex);
    lock->active_readers--;
    
    if (lock->active_readers == 0) {
        if (lock->waiting_supervisors > 0) pthread_cond_signal(&lock->cv_supervisors);
        else if (lock->waiting_managers > 0) pthread_cond_signal(&lock->cv_managers);
    }

    pthread_mutex_unlock(&lock->mutex);
}

static int dirlock_write_acquire(DirLock *lock, int is_supervisor) {
    pthread_mutex_lock(&lock->mutex);
    
    if (is_supervisor) lock->waiting_supervisors++;
    else lock->waiting_managers++;
    
    int preempts_manager = 0;
    
    while (lock->active_readers > 0 || lock->active_writers > 0 ||
           (is_supervisor == 0 && lock->waiting_supervisors > 0)) {
        if (is_supervisor) pthread_cond_wait(&lock->cv_supervisors, &lock->mutex);
        else pthread_cond_wait(&lock->cv_managers, &lock->mutex);
    }

    if (is_supervisor && lock->waiting_managers > 0) preempts_manager = 1;
    if (is_supervisor) lock->waiting_supervisors--;
    else lock->waiting_managers--;
    lock->active_writers++;

    pthread_mutex_unlock(&lock->mutex);
    return preempts_manager;
}

static void dirlock_write_release(DirLock *lock) {
    pthread_mutex_lock(&lock->mutex);
    lock->active_writers--;
    
    if (lock->waiting_supervisors > 0) pthread_cond_signal(&lock->cv_supervisors);
    else if (lock->waiting_managers > 0) pthread_cond_signal(&lock->cv_managers);
    else pthread_cond_broadcast(&lock->cv_readers);
    pthread_mutex_unlock(&lock->mutex);
}

static void* worker_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int worker_id = args->id;
    const char *filename = args->op.filename;
    
    int blocked_status = dirlock_read_acquire(args->lock);
    
    simulate_work(OP_Q2_WORKER_READ);
    
    int size = directory_get_size(args->dir, filename);
    
    pthread_mutex_lock(&print_mutex);
    if (blocked_status == -1) {
        printf("[Worker-%d] [worker blocked: supervisor pending] waiting...\n", worker_id);
        printf("[Worker-%d] FILE: %s SIZE: %d bytes\n", worker_id, filename, size);
    } else if (blocked_status == 1) {
        printf("[Worker-%d] [concurrent read] FILE: %s SIZE: %d bytes\n", worker_id, filename, size);
    } else {
        printf("[Worker-%d] FILE: %s SIZE: %d bytes\n", worker_id, filename, size);
    }
    pthread_mutex_unlock(&print_mutex);
    
    dirlock_read_release(args->lock);
    free(args);
    return NULL;
}

static void* manager_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int manager_id = args->id;
    const char *filename = args->op.filename;
    
    pthread_mutex_lock(&print_mutex);
    printf("[Manager-%d] waiting for write lock\n", manager_id);
    pthread_mutex_unlock(&print_mutex);
    
    int preempts = dirlock_write_acquire(args->lock, 0);
    
    simulate_work(OP_Q2_MANAGER_HANDLE);
    
    int new_size = 4096;
    directory_set_size(args->dir, filename, new_size);
    
    pthread_mutex_lock(&print_mutex);
    if (preempts) printf("[Manager-%d] [supervisor preempts manager] acquired write lock\n", manager_id);
    else printf("[Manager-%d] acquired write lock\n", manager_id);
    printf("[Manager-%d] updated %s → %d bytes\n", manager_id, filename, new_size);
    pthread_mutex_unlock(&print_mutex);
    
    dirlock_write_release(args->lock);
    free(args);
    return NULL;
}

static void* supervisor_thread(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int supervisor_id = args->id;
    const char *filename = args->op.filename;
    
    int preempts = dirlock_write_acquire(args->lock, 1);
    
    simulate_work(OP_Q2_SUPERVISOR_UPDATE);
    
    int new_size = 1024;
    directory_set_size(args->dir, filename, new_size);
    
    pthread_mutex_lock(&print_mutex);
    if (preempts) printf("[Supervisor-%d] [supervisor preempts manager] acquired write lock\n", supervisor_id);
    else printf("[Supervisor-%d] acquired write lock\n", supervisor_id);
    printf("[Supervisor-%d] updated %s → %d bytes\n", supervisor_id, filename, new_size);
    pthread_mutex_unlock(&print_mutex);
    
    dirlock_write_release(args->lock);
    free(args);
    return NULL;
}

typedef struct {
    int num_workers;
    int num_managers;
    int num_supervisors;
    int num_ops;
    Operation operations[MAX_OPERATIONS];
} SimulationConfig;

static int parse_input_file(const char *input_path, SimulationConfig *config) {
    FILE *fp = fopen(input_path, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }

    if (fscanf(fp, "%d %d %d %d", &config->num_workers, &config->num_managers,
               &config->num_supervisors, &config->num_ops) != 4) {
        fprintf(stderr, "Failed to parse header\n");
        fclose(fp);
        return -1;
    }
    
    for (int i = 0; i < config->num_ops; i++) {
        char role_char, op_char[16];
        int id;
        char filename[MAX_FILENAME_LEN];
        
        if (fscanf(fp, " %c %d %s %s", &role_char, &id, op_char, filename) != 4) {
            fprintf(stderr, "Failed to parse operation %d\n", i);
            fclose(fp);
            return -1;
        }
        
        ThreadRole role;
        if (role_char == 'W') role = ROLE_WORKER;
        else if (role_char == 'M') role = ROLE_MANAGER;
        else if (role_char == 'S') role = ROLE_SUPERVISOR;
        else {
            fprintf(stderr, "Unknown role: %c\n", role_char);
            fclose(fp);
            return -1;
        }
        
        OpType op;
        if (strcmp(op_char, "READ") == 0) op = OP_READ;
        else if (strcmp(op_char, "WRITE") == 0) op = OP_WRITE;
        else {
            fprintf(stderr, "Unknown operation: %s\n", op_char);
            fclose(fp);
            return -1;
        }
        
        config->operations[i].role = role;
        config->operations[i].id = id;
        config->operations[i].op = op;
        strcpy(config->operations[i].filename, filename);
    }
    
    fclose(fp);
    return 0;
}

/*
Purpose: Parse Q2 input file and run directory concurrency simulation.
Args: input_path - path to the Q2 operation file.
Return: 0 on success, non-zero on error.
*/
static int run_simulation(const char *input_path) {
    SimulationConfig config;
    if (parse_input_file(input_path, &config) != 0) {
        return 1;
    }
    
    Directory dir;
    directory_init(&dir);
    
    DirLock lock;
    dirlock_init(&lock);
    
    for (int i = 0; i < config.num_ops; i++) {
        directory_add_or_get(&dir, config.operations[i].filename);
    }
    
    pthread_t threads[MAX_OPERATIONS];
    int thread_count = 0;

    for (int i = 0; i < config.num_ops; i++) {
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (!args) {
            perror("malloc");
            return 1;
        }
        
        args->role = config.operations[i].role;
        args->id = config.operations[i].id;
        args->op = config.operations[i];
        args->dir = &dir;
        args->lock = &lock;
        
        void *(*thread_func)(void *) = NULL;
        if (args->role == ROLE_WORKER) thread_func = worker_thread;
        else if (args->role == ROLE_MANAGER) thread_func = manager_thread;
        else if (args->role == ROLE_SUPERVISOR) thread_func = supervisor_thread;
        
        if (pthread_create(&threads[thread_count], NULL, thread_func, args) != 0) {
            perror("pthread_create");
            free(args);
            return 1;
        }
        thread_count++;
    }
    
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    dirlock_destroy(&lock);
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
