/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "msg_queue.h"

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define CHILDREN_WAIT 0

sigset_t signalSet;
char runDirectory[PATH_MAX];
char *homeDirectory, *workingDirectory;

int N;

char *files_list[100];

char fifo1_path[PATH_MAX], fifo2_path[PATH_MAX];
int fifo1_fd, fifo2_fd;

int msg_queue_id;

int shmid, shm_flags_id;
message *shm_address;
bool *shm_flags_address;

int server_semid, client_semid;
unsigned short semInitVal[] = {0};

void dbprint(char *str){
    printf("[DEBUG] %s\n", str);
}

int create_sem_set(){
    int totSemaphores = sizeof(semInitVal)/sizeof(unsigned short);      // lunghezza array semafori
    
    // genero il set di semafori
    int semid = semget(IPC_PRIVATE, totSemaphores, IPC_CREAT | S_IRUSR | S_IWUSR);    
    if (semid == -1)
        ErrExit("semget failed");
    
    union semun arg;
    arg.array = semInitVal;
    if (semctl(semid, 0 /*ignored*/, SETALL, arg) == -1)    // imposto i valori iniziali dei semafori
        ErrExit("semctl SETALL failed");

    return semid;
}

void shm_write(message *msg){
    // richiede accesso ad array flags
    semOp(server_semid, SHM_FLAGS_SEM, -1, 0);

    int index;      // posizione in cui si puó scrivere
    if((index = findSHM(shm_flags_address, 0)) == -1)
        ErrExit("findSHM failed");
    
    // copia del messaggio in shared memory
    memcpy(&shm_address[index], msg, sizeof(message));

    // segnalo che la posizione é occupata
    shm_flags_address[index] = 1;

    // restituisce accesso ad array flags
    semOp(server_semid, SHM_FLAGS_SEM, 1, 0);
}

void ipcs_write(message * messages[4]){
    msgqueue_message msgq_msg;
    msgq_msg.mtype = 1;
    msgq_msg.payload = *(messages[2]);

    bool ipc_completed[] = {false, false, false, false};


    while(!ipc_completed[0] || !ipc_completed[1] || !ipc_completed[2] || !ipc_completed[3]){
        if(!ipc_completed[0]){
            errno = 0;
            semOp(server_semid, FIFO1_SEM, -1, IPC_NOWAIT);
            if(errno != EAGAIN){
                errno = 0;
                if(write(fifo1_fd, messages[0], sizeof(message)) == -1){    // message to fifo1
                    if(errno != EAGAIN)
                        ErrExit("fifo1 write failed");
                }
                else
                    ipc_completed[0] = true;
            }
        }

        if(!ipc_completed[1]){
            errno = 0;
            semOp(server_semid, FIFO2_SEM, -1, IPC_NOWAIT);
            if(errno != EAGAIN){
                errno = 0;
                if(write(fifo2_fd, messages[1], sizeof(message)) == -1){    // message to fifo2
                    if(errno != EAGAIN)
                        ErrExit("fifo2 write failed");
                }
                else
                    ipc_completed[1] = true;
            }
        }

        if(!ipc_completed[2]){
            errno = 0;
            semOp(server_semid, MSGQ_SEM, -1, IPC_NOWAIT);
            if(errno != EAGAIN){
                errno = 0;
                if (msgsnd(msg_queue_id, &msgq_msg, sizeof(msgqueue_message) - sizeof(long), IPC_NOWAIT) == -1){    // message to message queue
                    if(errno != EAGAIN)
                        ErrExit("msgsnd failed");
                    else
                        printf("[DEBUG] MESSAGE QUEUE FULL %s\n", msgq_msg.payload.filename); 
                }
                else {
                    printf("[DEBUG] MESSAGE QUEUE WRITE %s\n", msgq_msg.payload.filename); 
                    ipc_completed[2] = true;
                }
            }
        }

        if(!ipc_completed[3]){
            errno = 0;
            semOp(server_semid, SHM_SEM, -1, IPC_NOWAIT);
            if(errno != EAGAIN){
                shm_write(messages[3]);
                ipc_completed[3] = true;
            }
        }

        printf("%d) %s --> %d, %d, %d, %d\n", messages[0]->index, messages[0]->filename, ipc_completed[0], ipc_completed[1], ipc_completed[2], ipc_completed[3]);
    }

}

void child(int index){
    char * filepath = files_list[index];            // path file da leggere
    
    int charCount = getFileSize(filepath) - 1;      // dimensione totale del file
    if (charCount < 4)
        exit(0);

    int filePartsSize[4];                           // dimensione delle quattro parti del file

    // calcolo le dimensioni delle rispettive quattro porzioni di file
    int baseSize = (charCount % 4 == 0) ? (charCount / 4) : (charCount / 4 + 1);
    filePartsSize[0] = filePartsSize[1] = filePartsSize[2] = baseSize;    
    filePartsSize[3] = charCount - baseSize * 3;

    // apro il file
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd == -1)
	    ErrExit("error while opening file");

    // divido il file in 4 messaggi
    message * messages[4];
    for(int i = 0; i < 4; i++){
        messages[i] = (message *) malloc(sizeof(message));      // alloco lo spazio necessario
        messages[i]->index = index;
        messages[i]->pid = getpid();                // salvo il pid
        strcpy(messages[i]->filename, filepath);    // salvo il path del file
        read(file_fd, messages[i]->msg, sizeof(char) * filePartsSize[i]);       // salvo la porzione di file
    }
    close(file_fd);

    for(int i = 0; i < 4; i++){
        printf("--> %s\n\t%s\n", messages[i]->filename, messages[i]->msg);
    }

    semOp(client_semid, CHILDREN_WAIT, -1, 0);
    semOp(client_semid, CHILDREN_WAIT, 0, 0);      // aspetta tutti gli altri client

    dbprint("I client sono tutti pronti!");

    printf("Inizio a scrivere %s\n", messages[0]->filename);
    ipcs_write(messages);
    printf("Finito di scrivere %s\n", messages[0]->filename);

    for(int i = 0; i<4; i++)
        free(messages[i]);
    exit(0);
}

void sigIntHandler(){
    // modifico la maschera dei segnali
    sigfillset(&signalSet);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    // cambio directory di lavoro
    changeDir(workingDirectory);

    printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", getUsername(), workingDirectory); 

    // recupero N e la lista dei file da processare
    enumerate_dir(workingDirectory, &N, files_list);

    // set semafori del server
    server_semid = semget(SEMAPHORE_KEY, 6, S_IRUSR | S_IWUSR);

    dbprint("apro fifo1");
    if((fifo1_fd = open(fifo1_path, O_WRONLY)) == -1)       // apro fifo1 BLOCCANTE
        ErrExit("open fifo1 failed");
    if(write(fifo1_fd, &N, sizeof(int)) != sizeof(int))     // scrivo N su fifo1
        ErrExit("write fifo1 failed");                  
    close(fifo1_fd);                                        // chiudo fifo1
    dbprint("ho scritto N su fifo1");

    // Attende l'apertura dal server delle fifo in lettura (evita errore "ENXIO" / "No such device or address")
    semOp(server_semid, FIFO_READY, -1, 0);                 

    // fifo NON BLOCCANTI
    if((fifo1_fd = open(fifo1_path, O_WRONLY | O_NONBLOCK)) == -1)
        ErrExit("open fifo1 (nonblock) failed");
    if((fifo2_fd = open(fifo2_path, O_WRONLY | O_NONBLOCK)) == -1)
        ErrExit("open fifo2 (nonblock) failed");
    
    // message queue
    msg_queue_id = msgget(MSG_QUEUE_KEY, S_IRUSR | S_IWUSR);

    // shared memory
    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);
    shm_address = (message *) get_shared_memory(shmid, 0);

    // vettore di supporto alla shared memory
    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);
    shm_flags_address = (bool *) get_shared_memory(shm_flags_id, 0);

    if(server_semid == -1)
        ErrExit("semget error");

    semInitVal[0] = N;
    client_semid = create_sem_set();

    dbprint("attendo conferma su shared memory dal server");

    semOp(server_semid, (unsigned short)WAIT_DATA, -1, 0);      // attende scrittura del server su Shared Memory
    printf("Shared memory: %s\n", shm_address[0].msg);


    // genero N processi figli
    for(int i = 0; i<N; i++){
        pid_t pid = fork();
        switch (pid) {
            case -1:
                ErrExit("fork failed");
                break;
            case 0:
                child(i);
                break;
            default:
                break;
        }
    }

    // Attende la terminazione dei figli
    pid_t child;
    while ((child = wait(NULL)) != -1);


    close(fifo1_fd);
    close(fifo2_fd);
}

void sigUsr1Handler(){
    exit(0);
}

int main(int argc, char * argv[]) {

    // controllo il numero di parametri in input
    if (argc != 2) {
        printf("Usage: %s <HOME>/<directory>\n", argv[0]);
        return 1;
    }
    
    // recupero la directory in cui si trova l'eseguibile
    if (getcwd(runDirectory, PATH_MAX) == NULL)
        ErrExit("getcwd failed");

    // calcolo il full path delle due fifo (per potervi accedere anche dopo il cambio directory) 
    if(snprintf(fifo1_path, PATH_MAX, "%s/%s", runDirectory, FIFO1_NAME) < 0)
        ErrExit("snprintf error");
    if(snprintf(fifo2_path, PATH_MAX, "%s/%s", runDirectory, FIFO2_NAME) < 0)
        ErrExit("snprintf error");

    printf("%s\n", fifo1_path);

    // recupero la home directory
    homeDirectory = getHomeDir();

    // calcolo la directory di lavoro
    workingDirectory = strcat(homeDirectory, argv[1]);

    // while(true){
    sigfillset(&signalSet);
    sigdelset(&signalSet, SIGINT);
    sigdelset(&signalSet, SIGUSR1);

    // assegnazione del set alla maschera
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    // assegnazione degli handler ai rispettivi segnali
    if (signal(SIGINT, sigIntHandler) == SIG_ERR)
        ErrExit("sigint change signal handler failed");    
    if (signal(SIGUSR1, sigUsr1Handler) == SIG_ERR)
        ErrExit("sigusr1 change signal handler failed");

    pause();
    // }
}