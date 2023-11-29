#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define SHM_SIZE 1024

int shmId;
void *shmPtr;
pid_t childPid;

void SignalHandler(int sig) {
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shmPtr, sizeof(int));
        printf("First: Received signal SIGUSR1. Calculated sum is: %d\n", sum);
    }
}

void CreateSharedMemory() {
    shmId = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    shmPtr = shmat(shmId, NULL, 0);
    if (shmPtr == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}

void DestroySharedMemory() {
    shmdt(shmPtr);
    shmctl(shmId, IPC_RMID, NULL);
}

void RunChildProcess() {
    char shmIdStr[10];
    sprintf(shmIdStr, "%d", shmId);
    execlp("./second", "second", shmIdStr, NULL);
    perror("execlp");
    exit(1);
}

void HandleParentProcess() {
    while (1) {
        int n, i, input;
        printf("First: Enter the count of numbers to sum (0 to exit): ");
        scanf("%d", &n);
        if (n <= 0) {
            break;
        }
        for (i = 0; i < n; i++) {
            printf("First: Enter number %d: ", i + 1);
            scanf("%d", &input);
            memcpy(shmPtr + i * sizeof(int), &input, sizeof(int));
        }
        int endMarker = 0;
        memcpy(shmPtr + n * sizeof(int), &endMarker, sizeof(int));
        kill(childPid, SIGUSR1);
        pause();
    }
    kill(childPid, SIGTERM);
    waitpid(childPid, NULL, 0);
}

int main(int argc, char *argv[]) {
    CreateSharedMemory();
    signal(SIGUSR1, SignalHandler);
    childPid = fork();
    if (childPid == 0) {
        RunChildProcess();
    } else {
        HandleParentProcess();
        DestroySharedMemory();
    }
    return 0;
}