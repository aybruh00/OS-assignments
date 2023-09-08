#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>

pthread_mutex_t rng_mutex;

int thread_safe_rng(int min, int max) {
    pthread_mutex_lock(&rng_mutex);
    int r = rand();
    pthread_mutex_unlock(&rng_mutex);
    return min + r % max;
}

/* TODO : can add global vars, structs, functions etc */
sem_t junc[4];
sem_t cross_in[4];
sem_t cross_out[4];
int in_lock_held[4];
int out_lock_held[4];

enum direction {
    N, E, S, W
};

typedef struct trainD {
    enum direction dir;
    int id;
} trainD;

int num_trains;
int done;

trainD* trainDs;

void arriveLane(trainD* t_desc) {
    /* TODO: add code here */
    enum direction to = ((t_desc->dir) + 3)%4;

    sem_wait(&junc[t_desc->dir]);

    sem_wait(&cross_in[t_desc->dir]);
    in_lock_held[t_desc->dir] = t_desc->id;

    sem_wait(&cross_out[t_desc->dir]);
    out_lock_held[to] = t_desc->id;
}

void crossLane(trainD* t_desc) {
    /* TODO: add code here */
    enum direction to = ((t_desc->dir) + 3) % 4;

    sem_wait(&cross_out[to]);
    out_lock_held[to] = t_desc->id;

    usleep(1000 * thread_safe_rng(500, 1000)); // take 500-1000 ms to cross the lane
}

void exitLane(trainD* t_desc) {
    /* TODO: add code here */
    enum direction to = ((t_desc->dir) + 3) % 4;

    sem_post(&cross_out[t_desc->dir]);
    out_lock_held[t_desc->dir] = -2;

    sem_post(&cross_in[t_desc->dir]);
    in_lock_held[t_desc->dir] = -1;

    sem_post(&cross_out[to]);
    out_lock_held[to] = -2;
}


void* trainThreadFunction(void *arg)
{
    /* TODO extract arguments from the `void* arg` */
    char* train_dir;

    trainD* td = (trainD *) arg;
    usleep(thread_safe_rng(0, 10000)); // start at random time
    
    if (td->dir == N) {
        train_dir = "North";
    }
    else if (td->dir == E) {
        train_dir = "East";
    }
    else if (td->dir == S) {
        train_dir = "South";
    }
    else if (td->dir == W) {
        train_dir = "West";
    }


    arriveLane(td);
    printf("Train Arrived at the lane from the %s direction\n", train_dir);

    crossLane(td);


    printf("Train Exited the lane from the %s direction\n", train_dir);
    exitLane(td);

    return NULL;
}

void* deadLockResolverThreadFunction(void * t) {
    while (1) {
        if (done) break;

        /* TODO add code to detect deadlock and resolve if any */
        int deadLockDetected = 0; // TODO set to 1 if deadlock is detected

        int locked_trains = 0;
        
        for(int c = 0; c < 4; c++){
            // out lock c is required by train holding in lock (c+1)%4
            int o = c, i = (c + 1) % 4;
            if(out_lock_held[o] == -2){
                // break;
                sem_post(&cross_out[o]);
            }
            else if (in_lock_held[i] == out_lock_held[o]){
                locked_trains++;
            }
        }
        if (locked_trains == 4) deadLockDetected = 1;

        if (deadLockDetected) {
            printf("Deadlock detected. Resolving deadlock...\n");

            sem_post(&cross_out[0]);
            out_lock_held[0] = -2;
        }

        usleep(1000 * 500); // sleep for 500 ms
    }
    return NULL;
}






int main(int argc, char *argv[]) {


    srand(time(NULL));

    if (argc != 2) {
        printf("Usage: ./main <train dirs: [NSWE]+>\n");
        return 1;
    }

    pthread_mutex_init(&rng_mutex, NULL);
    sem_init(&junc[0], 0, 1);
    sem_init(&junc[1], 0, 1);
    sem_init(&junc[2], 0, 1);
    sem_init(&junc[3], 0, 1);

    sem_init(&cross_in[0], 0, 1);
    sem_init(&cross_in[1], 0, 1);
    sem_init(&cross_in[2], 0, 1);
    sem_init(&cross_in[3], 0, 1);
    
    sem_init(&cross_out[0], 0, 1);
    sem_init(&cross_out[1], 0, 1);
    sem_init(&cross_out[2], 0, 1);
    sem_init(&cross_out[3], 0, 1);
    
    memset(in_lock_held, -1, sizeof(int) * 4);
    memset(out_lock_held, -2, sizeof(int) * 4);

    pthread_t deadlock_thread;


    char* train = argv[1];
    num_trains = strlen(train);
    trainDs = malloc(sizeof(trainD) * num_trains);

    /* TODO create a thread for deadLockResolverThreadFunction */
    pthread_create(&deadlock_thread, NULL, &deadLockResolverThreadFunction, NULL);

    int train_num = 0;
    pthread_t train_threads[num_trains];

    done = 0;
    while (train[train_num] != '\0') {
        char train_dir = train[train_num];

        if (train_dir != 'N' && train_dir != 'S' && train_dir != 'E' && train_dir != 'W') {
            printf("Invalid train direction: %c\n", train_dir);
            printf("Usage: ./main <train dirs: [NSEW]+>\n");
            return 1;
        }
        if (train_dir=='N') {
            trainDs[train_num].dir = N;
            trainDs[train_num].id = train_num;
        }

        if (train_dir=='W') {
            trainDs[train_num].dir = W;
            trainDs[train_num].id = train_num;
        }

        if (train_dir=='S') {
            trainDs[train_num].dir = S;
            trainDs[train_num].id = train_num;
        }

        if (train_dir=='E') {
            trainDs[train_num].dir = E;
            trainDs[train_num].id = train_num;
        }

        /* TODO create a thread for the train using trainThreadFunction */
        pthread_create(&train_threads[train_num], NULL, &trainThreadFunction, (trainDs + train_num));
        train_num++;
    }

    /* TODO: join with all train threads*/
    for (int i = 0; i < num_trains; i++) {
        pthread_join(train_threads[i], NULL);
    }

    free(trainDs);
    
    done = 1;

    return 0;
}
