#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>

union Semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

void P(int semId) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = SEM_UNDO;
    if (semop(semId, &op, 1) == -1) {
        perror("Client: Semaphore P operation failed");
        exit(EXIT_FAILURE);
    }
}

void V(int semId) {
    struct sembuf op;
    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = SEM_UNDO;
    if (semop(semId, &op, 1) == -1) {
        perror("Client: Semaphore V operation failed");
        exit(EXIT_FAILURE);
    }
}

int shmId;
void *shmPtr;
int semId;

void SignalHandler(int sig) {
    printf("Client: Signal handler received signal %d\n", sig);
    if (sig == SIGUSR1) {
        int sum = 0, i = 0, val;

        printf("Client: Locking semaphore before reading data...\n");
        P(semId);

        do {
            memcpy(&val, (int *)shmPtr + i, sizeof(int));
            if (val == 0) break;
            sum += val;
            i++;
        } while (1);

        memcpy(shmPtr, &sum, sizeof(int));

        printf("Client: Releasing semaphore after reading...\n");
        V(semId);
        printf("Client: Sending signal back to server...\n");
        kill(getppid(), SIGUSR1);
    }
}

void ConnectSharedMemory(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <shm_id> <sem_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client: Connecting to shared memory...\n");
    shmId = atoi(argv[1]);
    semId = atoi(argv[2]);

    shmPtr = shmat(shmId, NULL, 0);
    if (shmPtr == (void *) -1) {
        perror("Client: Shared memory attachment failed");
        exit(EXIT_FAILURE);
    }

    printf("Client: Connecting to semaphore...\n");
    signal(SIGUSR1, SignalHandler);
}

void RunClient() {
    while (1) {
        pause();
    }
}

int main(int argc, char *argv[]) {
    ConnectSharedMemory(argc, argv);
    RunClient();
    shmdt(shmPtr);
    return 0;
}