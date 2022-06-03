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

message * messages[4];

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
    semOp(server_semid, SHM_MUTEX_SEM, -1, 0);

    int index;      // posizione in cui si puó scrivere
    if((index = findSHM(shm_flags_address, 0)) == -1)
        ErrExit("findSHM failed");
    
    // copia del messaggio in shared memory
    memcpy(&shm_address[index], msg, sizeof(message));

    // segnalo che la posizione é occupata
    shm_flags_address[index] = 1;

    // restituisce accesso ad array flags
    semOp(server_semid, SHM_MUTEX_SEM, 1, 0);
}

void ipcs_write(){
    msgqueue_message msgq_msg;
    msgq_msg.mtype = 1;
    msgq_msg.payload = *(messages[2]);

    bool valid = true;
    for(int i = 0; i<4; i++){
        if(strlen(messages[i]->msg) <= 0)
            valid = false;
    }
    if(!valid)
        ErrExit("Message length < 0!");

    bool fifo1, fifo2, msgq, shm;
    fifo1 = fifo2 = msgq = shm = false;

    while(!fifo1 || !fifo2 || !msgq || !shm){

        if(!fifo1){
            errno = 0;
            if(semOp(server_semid, FIFO1_SEM, -1, IPC_NOWAIT) == -1){
                if(errno != EAGAIN)
                    ErrExit("semop fifo1 failed");
            }
            else{
                if(write(fifo1_fd, messages[0], sizeof(message)) == sizeof(message)){    // message to fifo1
                    fifo1 = true;
                    printf("<client_%d> Ho scritto %s su FIFO1\n", getpid(), messages[0]->filename);
                }
                else
                    ErrExit("fifo1 failed");
            }
        }

        if(!fifo2){
            errno = 0;
            if(semOp(server_semid, FIFO2_SEM, -1, IPC_NOWAIT) == -1){
                if(errno != EAGAIN)
                    ErrExit("semop fifo2 failed");
            }
            else{
                if(write(fifo2_fd, messages[1], sizeof(message)) == sizeof(message)){    // message to fifo2
                    fifo2 = true;
                    printf("<client_%d> Ho scritto %s su FIFO2\n", getpid(), messages[1]->filename);
                }
                else
                    ErrExit("fifo2 failed");
            }
        }


        if(!msgq){
            errno = 0;
            if(semOp(server_semid, MSGQ_SEM, -1, IPC_NOWAIT) == -1){
                if(errno != EAGAIN)
                    ErrExit("semop msgq failed");
            }
            else{
                if (msgsnd(msg_queue_id, &msgq_msg, sizeof(msgqueue_message) - sizeof(long), 0) != -1){
                    msgq = true;
                    printf("<client_%d> Ho scritto %s su MSGQ\n", getpid(), messages[2]->filename);
                }
                else
                    ErrExit("msgq failed");
            }
        }
        
        if(!shm){
            errno = 0;
            if(semOp(server_semid, SHM_SEM, -1, IPC_NOWAIT) == -1){
                if(errno != EAGAIN)
                    ErrExit("semop shm failed");
            }
            else{
                shm_write(messages[3]);
                shm = true;
                printf("<client_%d> Ho scritto %s su SHM\n", getpid(), messages[3]->filename);
            }
        }
    }
}

void child(int index){
    char * filepath = files_list[index];            // path file da leggere
    
    
    off_t charCount = getFileSize(filepath);        // dimensione totale del file
    if (charCount < 4)
        exit(0);

    int filePartsSize[4];                           // dimensione delle quattro parti del file

    // calcolo le dimensioni delle rispettive quattro porzioni di file
    int baseSize = (charCount % 4 == 0) ? (charCount / 4) : ((charCount / 4) + 1);
    filePartsSize[0] = filePartsSize[1] = filePartsSize[2] = baseSize;    
    filePartsSize[3] = charCount - (baseSize * 3);

    // apro il file
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd == -1)
	    ErrExit("error while opening file");

    printf("<client_%d> Apro il file %s\n\tSuddivisione caratteri: %d %d %d %d\n", getpid(), filepath, filePartsSize[0], filePartsSize[1], filePartsSize[2], filePartsSize[3]);

    // divido il file in 4 messaggi
    for(int i = 0; i < 4; i++){
        messages[i] = (message *) malloc(sizeof(message));      // alloco lo spazio necessario
        messages[i]->index = index;
        messages[i]->pid = getpid();                // salvo il pid
        strcpy(messages[i]->filename, filepath);    // salvo il path del file

        if((read(file_fd, messages[i]->msg, sizeof(char) * filePartsSize[i])) != sizeof(char) * filePartsSize[i])
            ErrExit("couldn't read file properly");
    }
    close(file_fd);

    semOp(client_semid, CHILDREN_WAIT, -1, 0);
    semOp(client_semid, CHILDREN_WAIT,  0, 0);      // aspetta tutti gli altri client

    ipcs_write();

    for(int i = 0; i<4; i++)
        free(messages[i]);

    printf("<client_%d> Ho concluso\n", getpid());
    exit(0);
}

void sigIntHandler(){
    printf("<client_0> SIGINT ricevuto.\n");
    // modifico la maschera dei segnali
    sigfillset(&signalSet);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    // cambio directory di lavoro
    changeDir(workingDirectory);

    printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", getUsername(), workingDirectory); 

    // recupero N e la lista dei file da processare
    enumerate_dir(workingDirectory, &N, files_list);

    printf("<client_0> Ho trovato N=%d file sendme_\n", N);

    // set semafori del server

    if((fifo1_fd = open(fifo1_path, O_WRONLY)) == -1)       // apro fifo1 BLOCCANTE
        ErrExit("open fifo1 failed");
    printf("<client_0> Ho aperto FIFO1\n");

    if(write(fifo1_fd, &N, sizeof(int)) != sizeof(int))     // scrivo N su fifo1
        ErrExit("write fifo1 failed");
    printf("<client_0> Ho inviato N su FIFO1\n");         

    if((fifo2_fd = open(fifo2_path, O_WRONLY)) == -1)
        ErrExit("open fifo2 failed");
    printf("<client_0> Ho aperto FIFO2\n");


    server_semid = semget(SEMAPHORE_KEY, 6, S_IRUSR | S_IWUSR);
    if(server_semid == -1)
        ErrExit("semget error");
    printf("<client_0> Ho ricevuto il set di semafori del server \n");
    
    // message queue
    msg_queue_id = msgget(MSG_QUEUE_KEY, S_IRUSR | S_IWUSR);
    printf("<client_0> Ho aperto la MESSAGE QUEUE \n");

    // shared memory
    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);
    shm_address = (message *) get_shared_memory(shmid, 0);

    // vettore di supporto alla shared memory
    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);
    shm_flags_address = (bool *) get_shared_memory(shm_flags_id, 0);
    printf("<client_0> Ho inizializzato la SHARED MEMORY e il suo vettore di supporto \n");

    semInitVal[0] = N;
    client_semid = create_sem_set();
    printf("<client_0> Ho creato il set di semafori del client\n");

    printf("<client_0> Attendo il messaggio di conferma dal server su SHM\n");
    semOp(server_semid, WAIT_DATA, -1, 0);      // attende scrittura del server su Shared Memory
    printf("<client_0> Messaggio ricevuto su SHM: %s\n", shm_address[0].msg);

    printf("<client_0> Genero %d figli\n", N);
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

    printf("DIMENSIONE CHAR = %ld", sizeof(char));
    printf("DIMENSIONE MESSAGE = %ld bytes\n", sizeof(message));
    printf("DIMENSIONE PIPE_BUF = %d bytes\n", PIPE_BUF);

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

    printf("<client_0> Attendo SIGINT (ctrl+c) o SIGUSR1\n");
    pause();
    // }
}