/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "msg_queue.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <stdio.h>
#include <unistd.h>

int N;
sigset_t signalSet;
int fifo1_fd, fifo2_fd;
int semid, shmid;
int msg_queue_id;
message *shm_buffer;
unsigned short semInitVal[] = {0, 0, 50, 50, 50, 50};
message files_parts[100][4];

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

void remove_ipcs(){
    close(fifo1_fd);            // chiusura fifo1
    close(fifo2_fd);            // chiusura fifo2
    unlink(FIFO1_NAME);         // unlink fifo1
    unlink(FIFO2_NAME);         // unlink fifo2

    free_shared_memory(shm_buffer);     // svuota segmento di shared memory
    remove_shared_memory(shmid);        // elimina shared memory

    // Rimozione message queue
    if(msgctl(msg_queue_id, IPC_RMID, NULL) == -1)   // chiusura msg queue
        ErrExit("Message queue could not be deleted");

    // Rimozione set semafori
    if (semctl(semid, 0 /*ignored*/, IPC_RMID, NULL) == -1)
        ErrExit("semctl IPC_RMID failed");
    exit(0);
}


int main(int argc, char * argv[]){

    // creazione set di segnali
    sigfillset(&signalSet);
    sigdelset(&signalSet, SIGINT);

    // assegnazione del set alla maschera
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    if (signal(SIGINT, remove_ipcs) == SIG_ERR)
        ErrExit("sigint change signal handler failed");

    // crea fifo1, fifo2, set semafori, shared memory
    if (mkfifo(FIFO1_NAME, S_IRUSR | S_IWUSR) == -1)
        ErrExit("Creating FIFO1 failed");

    if (mkfifo(FIFO2_NAME, S_IRUSR | S_IWUSR) == -1)
        ErrExit("Creating FIFO2 failed");

    msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(msg_queue_id == -1)
        ErrExit("msgget failed");

    semid = create_sem_set();

    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);

    printf("[DEBUG] Lavoro su FIFO %s\n", FIFO1_NAME);

    fifo1_fd = open(FIFO1_NAME, O_RDONLY);
    fifo2_fd = open(FIFO2_NAME, O_RDONLY);

    if(read(fifo1_fd, &N, sizeof(N)) != sizeof(N))
        ErrExit("error while reading N from FIFO1");

    printf("[DEBUG] Ho letto N: %d\n", N);

    shm_buffer = (message *) get_shared_memory(shmid, 0);
    
    sprintf(shm_buffer[0].msg, "N is equal to %d", N);

    semOp(semid, (unsigned short)WAIT_DATA, 1);
    printf("[DEBUG] dati su SHM, sblocco il client\n");

    int received = 0;
    message fifo1_msg, fifo2_msg;
    msgqueue_message msgqueue_msg;


    while(received < N){
        if(read(fifo1_fd, &fifo1_msg, sizeof(message)) != sizeof(message))
            ErrExit("error while reading message from FIFO1");
        
        if(read(fifo2_fd, &fifo2_msg, sizeof(message)) != sizeof(message))
            ErrExit("error while reading message from FIFO2");

        if (msgrcv(msg_queue_id, &msgqueue_msg, sizeof(msgqueue_message) - sizeof(long), 0, 0) == -1)
            ErrExit("error while reading message from Message Queue");

        printf("[DEBUG] Reading from fifo1: [%s, %d, %s]\n", fifo1_msg.msg, fifo1_msg.pid, fifo1_msg.filename);
        printf("[DEBUG] Reading from fifo2: [%s, %d, %s]\n", fifo2_msg.msg, fifo2_msg.pid, fifo2_msg.filename);
        printf("[DEBUG] Reading from msgqueue: [%s, %d, %s]\n", msgqueue_msg.payload.msg, msgqueue_msg.payload.pid, msgqueue_msg.payload.filename);

        received++;
    }

    for(int i = 0; i<N; i++){
        printf("[DEBUG] Reading from shm: [%s, %d, %s]\n", shm_buffer[i].msg, shm_buffer[i].pid, shm_buffer[i].filename);
    }
    
    
    semOp(semid, (unsigned short)DATA_READY, -1);
    printf("[DEBUG] client ha finito, concludo...\n");

    remove_ipcs();
    return 0;
}
