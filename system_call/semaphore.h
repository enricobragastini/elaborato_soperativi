/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#include <sys/sem.h>
#include <sys/stat.h>

#define SEMAPHORE_KEY 1999

// server semaphores
#define WAIT_DATA   0
#define DATA_READY  1
#define FIFO1_SEM   2
#define FIFO2_SEM   3 
#define MSGQ_SEM    4
#define SHM_SEM     5

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO*/
};

void semOp (int semid, unsigned short sem_num, short sem_op);
void printSemaphoresValue (int semid);