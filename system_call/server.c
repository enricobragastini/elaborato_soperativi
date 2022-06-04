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

int fifo1_fd, fifo1_fd_extra, fifo2_fd, fifo2_fd_extra;

int msg_queue_id;

int shmid, shm_flags_id;
message *shm_buffer;
bool *shm_flags;

int N;

int semid;
unsigned short semInitVal[] = {0, 0, 50, 50, 50, 50, 1, 0};

static const char * ipcs_names[4] = {"FIFO1", "FIFO2", "msgQueue", "ShdMem"};

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
    printf("<server> CTRL+C ricevuto: elimino le IPC...\n");
    close(fifo1_fd);
    close(fifo1_fd_extra);
    close(fifo2_fd);
    close(fifo2_fd_extra);
    unlink(FIFO1_NAME);
    unlink(FIFO2_NAME);
    printf("<server> Termino.\n");
    exit(0);
}


void print_contatori(int * contatori){
    printf("Contatori: ");
    for(int i = 0; i<N; i++){
        printf("%d ", contatori[i]);
    } printf("\n");
}

void write_on_file(int row, message * files_parts[N][4]){
    char * path_in = files_parts[row][0]->filename;     // nome file input

    char path_out[PATH_MAX];                            // nome file output (con '_out')

    char * point;
    if((point = strrchr(path_in,'.')) == NULL )
        ErrExit("strrchr failed");

    strncpy(path_out, path_in, strlen(path_in)-4);
    path_out[strlen(path_in)-4] = '\0';
    strcat(path_out, "_out.txt");

    printf("<server> Scrivo il file %s\n", path_out);

    int fd = open(path_out, O_WRONLY | O_CREAT, 0644);
    if(fd == -1)
        ErrExit("error while opening output file");

    char * out_str;
    for(int i = 0; i<4; i++){
        int size = snprintf(NULL, 0, "[Parte %d, del file %s, spedita dal processo %d tramite %s]", 
                i, files_parts[row][i]->filename, files_parts[row][i]->pid, ipcs_names[i]) + 1;

        out_str = (char *) malloc(size * sizeof(char));

        snprintf(out_str, size, "[Parte %d, del file %s, spedita dal processo %d tramite %s]", 
                i, files_parts[row][i]->filename, files_parts[row][i]->pid, ipcs_names[i]);


        write(fd, out_str, strlen(out_str));
        write(fd, "\n", strlen("\n"));

        write(fd, &files_parts[row][i]->msg, strlen(files_parts[row][i]->msg));
        write(fd, "\n", strlen("\n"));

        if(i != 3)
            write(fd, "\n", strlen("\n"));

        free(out_str); 
    }    

    close(fd);
}

void update_parts(message * msg, int row, int ipc_index, message * files_parts[N][4], int * contatori){
    files_parts[row][ipc_index] = (message *) malloc(sizeof(message));
    memcpy(files_parts[row][ipc_index], msg, sizeof(message));

    contatori[row]++;

    if(contatori[row] == 4){                // Quando sono arrivate tutte le 4 parti
        write_on_file(row, files_parts);    // Scrivi su file
        for(int i = 0; i<4; i++)            
            free(files_parts[row][i]);      // Libera le aree di memoria occupate dai messaggi
    }

    if(contatori[row] > 4){
        printf("ROW %d --> CONTATORE = %d", row, contatori[row]);
        ErrExit("errore contatori");
    }
}


void ipcs_read(){
    if(N == 0)
        return;
    message * files_parts[N][4];     // struttura dati che accoglie i messaggi

    int contatori[N];               // conta quanti msg sono arrivati per ogni file: per poter scrivere su file quando ci sono tutti e 4
    memset(contatori, 0, sizeof(int) * N);


    size_t s_read;
    message fifo1_buffer, fifo2_buffer;
    msgqueue_message msgq_buffer;
    int index_shm;

    printf("<server> Inizio a ricevere i messaggi sulle IPC\n");

    int received = 0;
    while(received < (N*4)){
        
        errno = 0;
        s_read = read(fifo1_fd, &fifo1_buffer, sizeof(message));
        if(s_read == -1){
            if(errno != EAGAIN)
                ErrExit("fifo1 read failed");
        }
        else if(s_read == sizeof(message)){
            semOp(semid, FIFO1_SEM, 1, 0);
            printf("<server> Received message from %d on FIFO1\n", fifo1_buffer.pid);
            update_parts(&fifo1_buffer, fifo1_buffer.index, 0, files_parts, contatori);
            received++;
        }         

        errno = 0;
        s_read = read(fifo2_fd, &fifo2_buffer, sizeof(message));
        if(s_read == -1){
            if(errno != EAGAIN)
                ErrExit("fifo2 read failed");
        }
        else if(s_read == sizeof(message)){
            semOp(semid, FIFO2_SEM, 1, 0);
            printf("<server> Received message from %d on FIFO2\n", fifo2_buffer.pid);
            update_parts(&fifo2_buffer, fifo2_buffer.index, 1, files_parts, contatori);
            received++;
        }

        errno = 0;
        if(msgrcv(msg_queue_id, &msgq_buffer, sizeof(msgqueue_message) - sizeof(long), 0, IPC_NOWAIT) == -1){
            if(errno == ENOMSG)
                printf("<server> La Message Queue al momento é vuota (ENOMSG)\n");
            else if(errno != EAGAIN)
                ErrExit("msgq read failed");
        }
        else{
            semOp(semid, MSGQ_SEM, 1, 0);
            printf("<server> Received message from %d on MSGQ\n", msgq_buffer.payload.pid);
            update_parts(&msgq_buffer.payload, msgq_buffer.payload.index, 2, files_parts, contatori);
            received++;
        }

        printf("<server> Provo a leggere da SHM\n");
        if(semOp(semid, SHM_MUTEX_SEM, -1, IPC_NOWAIT) == -1){
            if(errno != EAGAIN)
                ErrExit("Shared Memory failed");
        }
        else{
            if((index_shm = findSHM(shm_flags, true)) != -1){
                printf("<server> Received message from %d on SHM\n", shm_buffer[index_shm].pid);
                update_parts(&shm_buffer[index_shm], shm_buffer[index_shm].index, 3, files_parts, contatori);
                semOp(semid, SHM_SEM, 1, 0);
                shm_flags[index_shm] = false;
                received++;
            }
            else
               printf("<server> La Shared Memory al momento é vuota\n");

            semOp(semid, SHM_MUTEX_SEM, 1, 0);
        }

        printf("[%d / %d]\n", received, (N*4));
    }
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
    printf("<server> Ho creato FIFO1 e FIFO2\n");

    // message queue
    if((msg_queue_id = msgget(MSG_QUEUE_KEY, IPC_CREAT | S_IRUSR | S_IWUSR | IPC_NOWAIT)) == -1)
        ErrExit("msgget failed");
    printf("<server> Ho creato la Message Queue\n");

    // shared memory
    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);
    shm_buffer = (message *) get_shared_memory(shmid, 0);

    // vettore di supporto alla shared memory
    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);
    shm_flags = (bool *) get_shared_memory(shm_flags_id, 0);
    printf("<server> Ho inizializzato la Shared Memory e il suo vettore di supporto\n");

    // set semafori
    semid = create_sem_set();
    printf("<server> Ho creato il set di semafori del server\n");


    while(true){
        if((fifo1_fd = open(FIFO1_NAME, O_RDONLY)) == -1)       // apro fifo1 BLOCCANTE
            ErrExit("open fifo1 failed");
        if(read(fifo1_fd, &N, sizeof(int)) != sizeof(int))     // leggo N da fifo1
            ErrExit("read fifo1 failed");                  
        close(fifo1_fd);                                        // chiudo fifo1
        printf("<server> Ho ricevuto N = %d dal client su FIFO1\n", N);

        if(N > 0){
            if((fifo1_fd = open(FIFO1_NAME, O_RDONLY | O_NONBLOCK)) == -1)       // apro fifo1 BLOCCANTE
                ErrExit("open fifo1 failed");
            if((fifo1_fd_extra = open(FIFO1_NAME, O_WRONLY | O_NONBLOCK)) == -1)
                ErrExit("open fifo1_extra (nonblock) failed");
            printf("<server> Ho aperto FIFO1 e FIFO1_EXTRA non bloccanti\n");

            if((fifo2_fd = open(FIFO2_NAME, O_RDONLY | O_NONBLOCK)) == -1)
                ErrExit("open fifo2 (nonblock) failed");
            if((fifo2_fd_extra = open(FIFO2_NAME, O_WRONLY | O_NONBLOCK)) == -1)
                ErrExit("open fifo2_extra (nonblock) failed");
            printf("<server> Ho aperto FIFO2 e FIFO2_EXTRA non bloccanti\n");

            for(int i = 0; i<50; i++)       // inizializzo il vettore di supporto
                shm_flags[i] = false;

            sprintf(shm_buffer[0].msg, "N is equal to %d", N);
            semOp(semid, WAIT_DATA, 1, 0);      // Scrittura su Shared Memory: sblocca il client
            printf("<server> Ho scritto il messaggio su Shared Memory\n");        

            ipcs_read();        
            semOp(semid, SERVER_DONE, 1, 0);

            msgqueue_message end_msg;
            end_msg.mtype = 1;
            strcpy(end_msg.payload.msg, "SERVER ENDED");
            if (msgsnd(msg_queue_id, &end_msg, sizeof(msgqueue_message) - sizeof(long), 0) != -1)
                printf("<server> Ho inviato il messaggio di fine su MSGQ\n");
            else
                ErrExit("msgsnd failed (end_msg)");

            close(fifo1_fd);
            close(fifo1_fd_extra);
            close(fifo2_fd);
            close(fifo2_fd_extra);
        }
    }
}
