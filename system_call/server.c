/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "msg_queue.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

int N;
sigset_t signalSet;
int fifo1_fd, fifo2_fd;
int semid, shmid, shm_flags_id;
int msg_queue_id;
message *shm_buffer;
bool *shm_flags;
unsigned short semInitVal[] = {0, 0, 50, 50, 50, 50, 1};

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


void write_on_file(int row, message * files_parts[N][4]){
    char * path_in = files_parts[row][0]->filename;        // full path file di input
    
    char path_out[PATH_MAX];                            // nome file output (con '_out')
    strcpy(path_out, path_in);
    strcat(path_out, "_out");

    FILE *file_out;
    if((file_out = fopen(path_out, "wt")) == NULL)
		ErrExit("fopen failed");

    char * ipcs[4] = {"FIFO1", "FIFO2", "msgQueue", "ShdMem"};

    for(int i = 0; i<4; i++){
        fprintf(file_out, "[Parte %d, del file %s, spedita dal processo %d tramite %s]\n%s\n", 
        i, path_in, files_parts[row][i]->pid, ipcs[i], files_parts[row][i]->msg);
        if(i != 3)
            fprintf(file_out, "\n");
        free(files_parts[row][i]);
    }
    fclose(file_out);
}

void update_parts(message * msg, int row, int ipc_index, message * files_parts[N][4], int * contatori, int * ipc_count){
    files_parts[row][ipc_index] = (message *) malloc(sizeof(message));
    memcpy(files_parts[row][ipc_index], msg, sizeof(message));
    // printf("file_parts: %s, msg: %s\n", files_parts[row][ipc_index]->msg, msg->msg);

    ipc_count[ipc_index]++;

    contatori[row]++;
    if(contatori[row] == 4)
        write_on_file(row, files_parts);
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
    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);

    printf("[DEBUG] Lavoro su FIFO %s\n", FIFO1_NAME);

    fifo1_fd = open(FIFO1_NAME, O_RDONLY);
    fifo2_fd = open(FIFO2_NAME, O_RDONLY);

    if(read(fifo1_fd, &N, sizeof(N)) != sizeof(N))
        ErrExit("error while reading N from FIFO1");

    printf("[DEBUG] Ho letto N: %d\n", N);

    shm_buffer = (message *) get_shared_memory(shmid, 0);
    shm_flags = (bool *) get_shared_memory(shm_flags_id, 0);

    for(int i = 0; i<50; i++)
        shm_flags[i] = false;
    
    sprintf(shm_buffer[0].msg, "N is equal to %d", N);

    semOp(semid, (unsigned short)WAIT_DATA, 1);
    printf("[DEBUG] dati su SHM, sblocco il client\n");


    message msg_buffer;
    msgqueue_message msgq_buffer;
    int index_shm;

    message * files_parts[N][4];
    int contatori[N];
    int ipc_count[4] = {0};

    memset(contatori, 0, sizeof(int) * N);

    while(ipc_count[0] < N || ipc_count[1] < N || ipc_count[2] < N || ipc_count[3] < N){
        if(ipc_count[0] < N){
            if(read(fifo1_fd, &msg_buffer, sizeof(message)) != sizeof(message))
                ErrExit("error while reading message from FIFO1");
            semOp(semid, FIFO1_SEM, 1);
            
            update_parts(&msg_buffer, msg_buffer.index, 0, files_parts, contatori, ipc_count);
        }
        if(ipc_count[1] < N){
            if(read(fifo2_fd, &msg_buffer, sizeof(message)) != sizeof(message))
                ErrExit("error while reading message from FIFO2");
            semOp(semid, FIFO2_SEM, 1);
            
            update_parts(&msg_buffer, msg_buffer.index, 1, files_parts, contatori, ipc_count);
        }
        if(ipc_count[2] < N){
            if (msgrcv(msg_queue_id, &msgq_buffer, sizeof(msgqueue_message) - sizeof(long), 0, 0) == -1)
                ErrExit("error while reading message from Message Queue");
            semOp(semid, MSGQ_SEM, 1);

            update_parts(&(msgq_buffer.payload), msgq_buffer.payload.index, 2, files_parts, contatori, ipc_count);
        }
        if(ipc_count[3] < N){
            semOp(semid, SHM_FLAGS_SEM, -1);
            if((index_shm = findSHM(shm_flags, true)) == -1){
                semOp(semid, SHM_FLAGS_SEM, 1);
                continue;
            }

            update_parts(&shm_buffer[index_shm], shm_buffer[index_shm].index, 3, files_parts, contatori, ipc_count);

            shm_flags[index_shm] = false;
            semOp(semid, SHM_FLAGS_SEM, 1);
            semOp(semid, SHM_SEM, 1);
        }
    }

    printf("N: %d\n", N);
    for(int r = 0; r<N; r++){
        printf("riga %d\n", r);
        for(int i = 0; i<4; i++){
            printf("%d) pid: %d, filename: %s, msg: %s\n", i, files_parts[r][i]->pid, files_parts[r][i]->filename, files_parts[r][i]->msg);
        }
        printf("\n\n");
    }    
    
    semOp(semid, (unsigned short)DATA_READY, -1);
    printf("[DEBUG] client ha finito, concludo...\n");

    remove_ipcs();
    return 0;
}
