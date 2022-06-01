/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "msg_queue.h"

#include <bits/types/sigset_t.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define CHILDREN_WAIT 0

sigset_t signalSet;
char *homeDirectory;
char *workingDirectory;
int N;
int server_semid;
int fifo1_fd, fifo2_fd;
int msg_queue_id;
int shmid, shm_flags_id;
bool *shm_flags_address;
message *shm_address;
char *files_list[100];

int client_semid;
unsigned short semInitVal[] = {0};

void shm_write(message *msg){
    // richiede accesso ad array flags
    semOp(server_semid, SHM_FLAGS_SEM, -1);

    int index;      // posizione in cui si puó scrivere
    if((index = findSHM(shm_flags_address, 0)) == -1)
        ErrExit("findSHM failed");
    
    // copia del messaggio in shared memory
    memcpy(&shm_address[index], msg, sizeof(message));

    // segnalo che la posizione é occupata
    shm_flags_address[index] = 1;

    // restituisce accesso ad array flags
    semOp(server_semid, SHM_FLAGS_SEM, 1);
}

void child(int index){
    char * filepath = files_list[index];            // file da leggere

    int charCount = getFileSize(filepath) - 1;      // dimensione totale del file
    if (charCount == 0)
        exit(0);

    int filePartsSize[4];      // dimensione delle quattro parti del file

    // TODO: cosa succede se charCount < 4 ?

    // calcolo le dimensioni delle rispettive quattro porzioni di file
    int baseSize = (charCount % 4 == 0) ? (charCount / 4) : (charCount / 4 + 1);
    filePartsSize[0] = filePartsSize[1] = filePartsSize[2] = baseSize;    
    filePartsSize[3] = charCount - baseSize * 3;

    // apro il file
    int file_fd = open(filepath, O_RDONLY);
    if (file_fd == -1)
	    ErrExit("error while opening file");

    // divido il file nelle quattro parti
    message * messages[4];
    for(int i = 0; i < 4; i++){
        messages[i] = (message *) malloc(sizeof(message));      // alloco lo spazio necessario

        messages[i]->index = index;
        messages[i]->pid = getpid();                // salvo il pid
        strcpy(messages[i]->filename, filepath);    // salvo il path del file
        read(file_fd, messages[i]->msg, sizeof(char) * filePartsSize[i]);       // salvo la porzione di file
    }

    printf("[DEBUG] Sono il figlio: %d (index = %d)\n[DEBUG] Apro il file: %s\n\tparte 1: %s\n\tparte 2: %s\n\tparte 3: %s\n\tparte 4: %s\n\n", 
            messages[0]->pid, index, messages[0]->filename, messages[0]->msg, messages[1]->msg, messages[2]->msg, messages[3]->msg);
    
    semOp(client_semid, CHILDREN_WAIT, -1);    
    semOp(client_semid, CHILDREN_WAIT, 0);
    printf("[DEBUG] I client sono pronti all'invio dei file!\n");

    semOp(server_semid, FIFO1_SEM, -1);
    if(write(fifo1_fd, messages[0], sizeof(message)) != sizeof(message))    // message to fifo1
        ErrExit("fifo1 write() failed");

    semOp(server_semid, FIFO2_SEM, -1);
    if(write(fifo2_fd, messages[1], sizeof(message)) != sizeof(message))    // message to fifo2
        ErrExit("fifo2 write() failed");

    msgqueue_message msgq_msg;
    msgq_msg.mtype = 1;
    msgq_msg.payload = *(messages[2]);

    semOp(server_semid, MSGQ_SEM, -1);
    if (msgsnd(msg_queue_id, &msgq_msg, sizeof(msgqueue_message) - sizeof(long), 0) == -1)                    // message to message queue
        ErrExit("msgsnd failed");
    
    semOp(server_semid, SHM_SEM, -1);
    shm_write(messages[3]);

    for(int i = 0; i < 4; i++)
        free(messages[i]);
    close(file_fd);
    exit(0);
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

void sigIntHandler(){
    printf("[DEBUG] signale SIGINT ricevuto: aggiorno la maschera dei segnali\n");

    // Aggiorno la maschera dei segnali (li comprende tutti)
    sigfillset(&signalSet);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    // apre la fifo
    fifo1_fd = open(FIFO1_NAME, O_WRONLY);
    fifo2_fd = open(FIFO2_NAME, O_WRONLY);

    // cambia directory di lavoro
    printf("[DEBUG] cambio cartella di lavoro\n");
    changeDir(workingDirectory);

    printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", getUsername(), workingDirectory); 

    // conta file con cui lavorare e salva i relativi filename nell'array
    N = 0;
    enumerate_dir(workingDirectory, &N, files_list);

    printf("\n[DEBUG] Ho contato N=%d files\n", N);

    // Scrive N su fifo1
    write(fifo1_fd, &N, sizeof(N));

    // shared memory
    shmid = alloc_shared_memory(SHM_KEY, SHM_SIZE);
    shm_address = (message *) get_shared_memory(shmid, 0);

    shm_flags_id = alloc_shared_memory(SHM_FLAGS_KEY, SHM_FLAGS_SIZE);
    shm_flags_address = (bool *) get_shared_memory(shm_flags_id, 0);

    msg_queue_id = msgget(MSG_QUEUE_KEY, S_IRUSR | S_IWUSR);

    server_semid = semget(SEMAPHORE_KEY, 6, S_IRUSR | S_IWUSR);
    if(server_semid == -1)
        ErrExit("semget error");

    // Attende conferma dal server su shared memory
    semOp(server_semid, (unsigned short)WAIT_DATA, -1);
    printf("[DEBUG] Leggo da Shared Memory: \"%s\"\n\n", shm_address[0].msg);

    semInitVal[0] = N;
    client_semid = create_sem_set();

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

    // Aspetta la terminazione dei figli
    pid_t child;
    while ((child = wait(NULL)) != -1);

    close(fifo1_fd);
    printf("[DEBUG] Ho finito, avviso il server.\n");
    semOp(server_semid, (unsigned short)DATA_READY, 1);
}

void sigUsr1Handler(){
    printf("[DEBUG] signale SIGUSR1 ricevuto: chiudo baracca e burattini...\n");
    exit(0);
}

int main(int argc, char * argv[]) {

    // controllo il numero di parametri in input
    if (argc != 2) {
        printf("Usage: %s <HOME>/<directory>\n", argv[0]);
        return 1;
    }
    
    printf("[DEBUG] calcolo il path della working directory\n");
    homeDirectory = getHomeDir();
    workingDirectory = strcat(homeDirectory, argv[1]);


    // creazione set di segnali
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

    // aspetta un segnale
    pause();

    return 0;
}
