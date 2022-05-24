/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <stdio.h>
#include <unistd.h>

int N;
int fifo1_fd;
int semid;
unsigned short semInitVal[] = {0, 0};

int create_sem_set(){
    int totSemaphores = sizeof(semInitVal)/sizeof(unsigned short);      // lunghezza array semafori
    
    // genero il set di semafori
    int semid = semget(SEMAPHORE_KEY, totSemaphores, IPC_CREAT | S_IRUSR | S_IWUSR);    
    if (semid == -1)
        ErrExit("semget failed");
    
    union semun arg;
    arg.array = semInitVal;
    if (semctl(semid, 0 /*ignored*/, SETALL, arg) == -1)    // imposto i valori iniziali dei semafori
        ErrExit("semctl SETALL failed");

    return semid;
}

int main(int argc, char * argv[]){

    // crea fifo1, fifo2, set semafori, shared memory
    if (mkfifo(FIFO1_NAME, S_IRUSR | S_IWUSR) == -1)
        ErrExit("Creating FIFO1 failed");

    // if (mkfifo(FIFO2_NAME, S_IRUSR | S_IWUSR) == -1)
    //     ErrExit("Creating FIFO2 failed");

    int semid = create_sem_set();
    int shmid = alloc_shared_memory(SHM_KEY, sizeof(char)*50);

    printf("[DEBUG] Lavoro su FIFO %s\n", FIFO1_NAME);

    fifo1_fd = open(FIFO1_NAME, O_RDONLY);
    if(read(fifo1_fd, &N, sizeof(N)) != sizeof(N))
        ErrExit("error while reading N from FIFO1");

    printf("[DEBUG] Ho letto N: %d\n", N);

    char *shm_buffer = get_shared_memory(shmid, 0);
    
    sprintf(shm_buffer, "N is equal to %d", N);

    semOp(semid, (unsigned short)WAIT_DATA, 1);
    printf("[DEBUG] dati su SHM, sblocco il client\n");

    semOp(semid, (unsigned short)DATA_READY, -1);
    printf("[DEBUG] client ha letto, concludo...\n");

    close(fifo1_fd);            // chiusura fifo1
    // close(fifo2_fd);            // chiusura fifo2
    unlink(FIFO1_NAME);     // unlink fifo1
    // unlink(FIFO2_NAME);     // unlink fifo2

    free_shared_memory(shm_buffer);     // svuota segmento di shared memory
    remove_shared_memory(shmid);        // elimina shared memory

    // Eliminazione set semafori
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        ErrExit("semctl IPC_RMID failed");
    return 0;
}
