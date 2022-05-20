/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"

#include <errno.h>
#include <stdio.h>

void semOp(int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1){
        printf("errno: %d\n", errno);
        ErrExit("semop failed");
    }
}

void printSemaphoresValue (int semid) {
    unsigned short semVal[2];
    union semun arg;
    arg.array = semVal;

    // get the current state of the set
    if (semctl(semid, 0 /*ignored*/, GETALL, arg) == -1)
        ErrExit("semctl GETALL failed");

    // print the semaphore's value
    printf("semaphore set state:\n");
    for (int i = 0; i < 2; i++)
        printf("id: %d --> %d\n", i, semVal[i]);
}