#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define SHM_SIZE 1024

union Semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

int shmId;
void *shmPtr;
pid_t childPid;
int semId;

void SignalHandler(int sig) {
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shmPtr, sizeof(int));
        printf("Server: Sum calculated by client is: %d\n", sum);
    }
}

void InitSemaphore(int semId, int semValue) {
    union Semun argument;
    argument.val = semValue;
    if (semctl(semId, 0, SETVAL, argument) == -1) {
        perror("semctl");
        exit(1);
    }
}

void P(int semId) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;
    if (semop(semId, operations, 1) == -1) {
        perror("semop P");
        exit(1);
    }
}

void V(int semId) {
    struct sembuf operations[1];
    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;
    if (semop(semId, operations, 1) == -1) {
        perror("semop V");
        exit(1);
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

void CreateSemaphore() {
    semId = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semId == -1) {
        perror("semget");
        exit(1);
    }

    InitSemaphore(semId, 1);
}

void Cleanup() {
    shmdt(shmPtr);
    shmctl(shmId, IPC_RMID, NULL);
    semctl(semId, 0, IPC_RMID, 0);
}

void RunChildProcess() {
    char shmIdStr[10];
    char semIdStr[10];
    sprintf(shmIdStr, "%d", shmId);
    sprintf(semIdStr, "%d", semId);

    execlp("./second", "second", shmIdStr, semIdStr, (char *)NULL);

    perror("execlp");
    exit(1);
}

void HandleInput() {
    while (1) {
        int n, i, input;
        printf("Server: Enter the quantity of numbers to sum (0 - exit): ");
        scanf("%d", &n);

        if (n <= 0) {
            break;
        }

        printf("Server: Locking semaphore before writing data...\n");
        P(semId);

        for (i = 0; i < n; i++) {
            printf("Server: Enter the number: ");
            scanf("%d", &input);
            memcpy((int *)shmPtr + i, &input, sizeof(int));
        }

        printf("Server: Releasing semaphore after writing...\n");
        V(semId);

        printf("Server: Sending signal to child process...\n");
        kill(childPid, SIGUSR1);

        printf("Server: Waiting for response from child...\n");
        pause();
    }
}

int main() {
    CreateSharedMemory();
    CreateSemaphore();

    signal(SIGUSR1, SignalHandler);

    printf("Server: Creating child process...\n");
    childPid = fork();

    if (childPid == 0) {
        RunChildProcess();
    } else {
        HandleInput();

        kill(childPid, SIGTERM);
        waitpid(childPid, NULL, 0);
        Cleanup();
    }

    return 0;
}