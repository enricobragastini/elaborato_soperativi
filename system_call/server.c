/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "msg_queue.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include <bits/types/sigset_t.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

sigset_t signalSet;

int fifo1_fd, fifo2_fd;

int msg_queue_id;

int shmid, shm_flags_id;
message *shm_buffer;
bool *shm_flags;

int N;

int semid;
unsigned short semInitVal[] = {0, 0, 50, 50, 50, 50, 1};

static const char * ipcs_names[4] = {"FIFO1", "FIFO2", "msgQueue", "ShdMem"};

void dbprint(char *str){
    //printf("[DEBUG] %s\n", str);
}

int create_sem_set(){
    int totSemaphores = sizeof(semInitVal)/sizeof(unsigned short);      // lunghezza array semafori
    
    // genero il set di semafori
    int sem_set_id = semget(SEMAPHORE_KEY, totSemaphores, IPC_CREAT | S_IRUSR | S_IWUSR);    
    if (sem_set_id == -1)
        ErrExit("semget failed");
    
    union semun arg;
    arg.array = semInitVal;
    if (semctl(sem_set_id, 0 /*ignored*/, SETALL, arg) == -1)    // imposto i valori iniziali dei semafori
        ErrExit("semctl SETALL failed");

    return sem_set_id;
}

void remove_ipcs(){
    close(fifo1_fd);
    close(fifo2_fd);
    unlink(FIFO1_NAME);
    unlink(FIFO2_NAME);
}


void print_contatori(int * contatori){
    printf("Contatori: ");
    for(int i = 0; i<N; i++){
        printf("%d ", contatori[i]);
    } printf("\n");
}

void write_on_file(int row, message * files_parts[N][4]){
    char * path_in = files_parts[row][0]->filename;     // nome file input
    
    printf("(Row %d) Scrivo il file %s\n", row, path_in);

    char path_out[PATH_MAX];                            // nome file output (con '_out')
    strcpy(path_out, path_in);
    strcat(path_out, "_out");

    printf("(Row %d) Scrivo il file %s\n", row, path_out);

    int fd = open(path_out, O_WRONLY | O_CREAT, 0644);
    if(fd == -1)
        ErrExit("error while opening output file");

    char * out_str;
    for(int i = 0; i<4; i++){
        printf("\n--> %d\n", i);
        //printf("[DEBUG] Calcolo la dimensione della stringa (%s)\n", files_parts[row][i]->filename);
        int size = snprintf(NULL, 0, "[Parte %d, del file %s, spedita dal processo %d tramite %s]", 
                i, files_parts[row][i]->filename, files_parts[row][i]->pid, ipcs_names[i]);

        //printf("[DEBUG] Alloco %d char\n", size);
        out_str = (char *) malloc(size * sizeof(char));

        //printf("[DEBUG] Scrivo la stringa sul buffer %p\n", out_str);
        snprintf(out_str, size, "[Parte %d, del file %s, spedita dal processo %d tramite %s]", 
                i, files_parts[row][i]->filename, files_parts[row][i]->pid, ipcs_names[i]);

        //printf("[DEBUG] Ho scritto la stringa sul buffer: %s\n", out_str);

        write(fd, out_str, strlen(out_str));
        write(fd, "\n", strlen("\n"));

        write(fd, &files_parts[row][i]->msg, strlen(files_parts[row][i]->msg));
        write(fd, "\n", strlen("\n"));

        //printf("[DEBUG] Ho scritto la parte %d del file\n", i);

        if(i != 3)
            write(fd, "\n", strlen("\n"));

        free(out_str); 
    }

    for(int i = 0; i<4; i++)
        free(files_parts[row][i]);

    close(fd);
    // exit(0);
}

void update_parts(message * msg, int row, int ipc_index, message * files_parts[N][4], int * contatori, int * ipc_count){
    files_parts[row][ipc_index] = (message *) malloc(sizeof(message));
    memcpy(files_parts[row][ipc_index], msg, sizeof(message));

    ipc_count[ipc_index]++;
    contatori[row]++;

    print_contatori(contatori);

    if(contatori[row] == 4){
        //printf("[DEBUG] Ho tutti i %d pezzi di %s\n", contatori[row], files_parts[row][ipc_index]->filename);
        write_on_file(row, files_parts);

        // pid_t pid = fork();
        // switch (pid) {
        //     case -1:
        //         ErrExit("fork failed");
        //         break;
        //     case 0:
        //         write_on_file(row, files_parts);
        //         break;
        //     default:
        //         break;
        // }
    }

    if(contatori[row] > 4){
        ErrExit("errore contatori");
    }
}

void ipcs_read(){
    if(N == 0)
        return;
    message * files_parts[N][4];     // struttura dati che accoglie i messaggi

    int contatori[N];               // conta quanti msg sono arrivati per ogni file: per poter scrivere su file quando ci sono tutti e 4
    memset(contatori, 0, sizeof(int) * N);

    print_contatori(contatori);

    int ipc_count[4] = {0};         // conta quanti msg sono arrivati per ogni ipc: per poter smettere quando ne sono arrivati N

    message msg_buffer;
    msgqueue_message msgq_buffer;
    int index_shm;

    dbprint("Inizio a ricevere messaggi dalle IPC");

    while(ipc_count[0] < N || ipc_count[1] < N || ipc_count[2] < N || ipc_count[3] < N){

        printf("%d %d %d %d\n", ipc_count[0], ipc_count[1], ipc_count[2], ipc_count[3]);

        if(ipc_count[0] < N){
            errno = 0;
            if(read(fifo1_fd, &msg_buffer, sizeof(message)) == -1){
                if(errno != EAGAIN)
                    ErrExit("error while reading message from FIFO1");
            }
            if(errno != EAGAIN){
                //printf("[DEBUG] Leggo da fifo1\n");
                semOp(semid, FIFO1_SEM, 1, 0);
                printf("Indice %d, FIFO1\n", msg_buffer.index);
                update_parts(&msg_buffer, msg_buffer.index, 0, files_parts, contatori, ipc_count);
            }            
        }

        if(ipc_count[1] < N){
            errno = 0;
            if(read(fifo2_fd, &msg_buffer, sizeof(message)) == -1){
                if(errno != EAGAIN)
                    ErrExit("error while reading message from FIFO2");
            }
            if(errno != EAGAIN){
                //printf("[DEBUG] Leggo da fifo2\n");
                semOp(semid, FIFO2_SEM, 1, 0);
                printf("Indice %d, FIFO2\n", msg_buffer.index);
                update_parts(&msg_buffer, msg_buffer.index, 1, files_parts, contatori, ipc_count);
            }            
        }

        if(ipc_count[2] < N){
            dbprint("Provo a leggere da msgqueue");
            errno = 0;
            if(msgrcv(msg_queue_id, &msgq_buffer, sizeof(msgqueue_message) - sizeof(long), 0, IPC_NOWAIT) == -1){
                if(errno != ENOMSG)
                    ErrExit("error while reading message from Message Queue");
                else
                    dbprint("Message Queue ENOMSG");
            }
            else{
                //printf("[DEBUG] Leggo da Message Queue\n");
                semOp(semid, MSGQ_SEM, 1, 0);
                printf("Indice %d, MSGQ\n", msgq_buffer.payload.index);
                update_parts(&(msgq_buffer.payload), msgq_buffer.payload.index, 2, files_parts, contatori, ipc_count);
            }            
        }

        if(ipc_count[3] < N){
            dbprint("Provo a leggere da Shared Memory");
            semOp(semid, SHM_FLAGS_SEM, -1, 0);
            if((index_shm = findSHM(shm_flags, true)) == -1){
                dbprint("Niente da leggere in Shared Memory");
                semOp(semid, SHM_FLAGS_SEM, 1, 0);
                continue;
            }
            else{
                printf("Indice %d, SHM\n", shm_buffer[index_shm].index);
                update_parts(&shm_buffer[index_shm], shm_buffer[index_shm].index, 3, files_parts, contatori, ipc_count);
                //printf("[DEBUG] Leggo da Shared Memory!\n");
                semOp(semid, SHM_SEM, 1, 0);
                shm_flags[index_shm] = false;
                semOp(semid, SHM_FLAGS_SEM, 1, 0);
            }       
        }

    }

    // pid_t child;
    // while ((child = wait(NULL)) != -1);

}

int main(int argc, char * argv[]) {
    
    // set di segnali    
    sigfillset(&signalSet);
    sigdelset(&signalSet, SIGINT);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);
    if (signal(SIGINT, remove_ipcs) == SIG_ERR)
        ErrExit("sigint change signal handler failed");


    unlink(FIFO1_NAME);
    unlink(FIFO2_NAME);

    if (mkfifo(FIFO1_NAME, S_IRUSR | S_IWUSR) == -1)    // creo fifo1
        ErrExit("Creating FIFO1 failed");

    if (mkfifo(FIFO2_NAME, S_IRUSR | S_IWUSR) == -1)    // creo fifo2
        ErrExit("Creating FIFO2 failed");

    // message queue
    if((msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | S_IRUSR | S_IWUSR | IPC_NOWAIT)) == -1)
        ErrExit("msgget failed");

    // shared memory
    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);
    shm_buffer = (message *) get_shared_memory(shmid, 0);

    // vettore di supporto alla shared memory
    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);
    shm_flags = (bool *) get_shared_memory(shm_flags_id, 0);

    // set semafori
    semid = create_sem_set();

    dbprint("ipc create");


    // while(true){
        dbprint("apro fifo1");
        if((fifo1_fd = open(FIFO1_NAME, O_RDONLY)) == -1)       // apro fifo1 BLOCCANTE
            ErrExit("open fifo1 failed");
        if(read(fifo1_fd, &N, sizeof(int)) != sizeof(int))     // leggo N da fifo1
            ErrExit("read fifo1 failed");                  
        close(fifo1_fd);                                        // chiudo fifo1

        printf("N = %d\n", N);
        dbprint("N ricevuto!");

        if((fifo1_fd = open(FIFO1_NAME, O_RDONLY | O_NONBLOCK)) == -1)
            ErrExit("open fifo1 (nonblock) failed");
        if((fifo2_fd = open(FIFO2_NAME, O_RDONLY | O_NONBLOCK)) == -1)
            ErrExit("open fifo2 (nonblock) failed");
        
        // conferma apertura in lettura delle FIFO 
        semOp(semid, FIFO_READY, 1, 0);

        for(int i = 0; i<50; i++)       // inizializzo il vettore di supporto
            shm_flags[i] = false;

        sprintf(shm_buffer[0].msg, "N is equal to %d", N);
        dbprint("Ho scritto su Shared Memory!");
        semOp(semid, (unsigned short)WAIT_DATA, 1, 0);      // Scrittura su Shared Memory: sblocca il client

        ipcs_read();

        dbprint("Ho finito di leggere...");
    // }

    remove_ipcs();
}
