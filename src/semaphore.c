#include <sys/sem.h>
#include <sys/ipc.h>
#include <stdio.h>

int semid;  // global definition

void init_semaphore() {
    key_t key = ftok("/etc/hosts", 'S');//FTOK used to generate a key_t type
    if (key == -1) {
        perror("ftok failed");
        return;
    }

    semid = semget(key, 1, IPC_CREAT | 0666);// Create semaphore for index list protection from concurrent change; 

    if (semid == -1) {
        perror("semget failed");
        return;
    }

    union semun {
        int val;
    } arg;

    arg.val = 1;  // //set value 1 for future unlock state
    if (semctl(semid, 0, SETVAL, arg) == -1) {// Set semaphore #0 to 1 (unlocked)
        perror("semctl failed");
    }
}
