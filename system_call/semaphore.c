/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"
#include "defines.h"

#include <errno.h>
#include <stdio.h>

void semOp(int semid, unsigned short sem_num, short sem_op, int sem_flg) {
    struct sembuf sop = {
        .sem_num = sem_num, 
        .sem_op = sem_op, 
        .sem_flg = sem_flg
    };

    if (semop(semid, &sop, 1) == -1){
        printf("errno: %d\n", errno);
        ErrExit("semop failed");
    }
}
