#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <string.h>

int shmId;
void *shmPtr;

void SignalHandler(int sig) {
    printf("\nSecond: Signal handler called with signal: %d\n", sig);

    if (sig == SIGUSR1) {
        int sum = 0, i = 0, val;
        printf("Second: Reading data from shared memory...\n");

        do {
            memcpy(&val, shmPtr + i * sizeof(int), sizeof(int));
            if (val == 0) break;
            sum += val;
            i++;
        } while (1);

        printf("Second: Calculated sum: %d\n", sum);

        memcpy(shmPtr, &sum, sizeof(int));

        kill(getppid(), SIGUSR1);
        printf("Second: Sent signal SIGUSR1 back to the parent\n");
    }
}

void AttachSharedMemory(char *shmIdStr) {
    shmId = atoi(shmIdStr);
    printf("\nSecond: Received shared memory ID: %d\n", shmId);
    shmPtr = shmat(shmId, NULL, 0);

    if (shmPtr == (void *) -1) {
        perror("Second: shmat");
        exit(1);
    } else {
        printf("Second: Attached shared memory at address: %p\n", shmPtr);
    }
}

void RegisterSignalHandler() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigaction(SIGUSR1, &sa, NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <shm_id>\n", argv[0]);
        return 1;
    }

    AttachSharedMemory(argv[1]);
    RegisterSignalHandler();

    while (1) {
        pause();
    }

    shmdt(shmPtr);
    return 0;
}