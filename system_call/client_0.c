/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

#include <bits/types/sigset_t.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

sigset_t signalSet;
char *homeDirectory;
char *workingDirectory;
int N;
int fifo1_fd;
char *files_list[100];

void child(int index){
    printf("[DEBUG] Sono il figlio: %d (index = %d)\n", getpid(), index);
    printf("[DEBUG] Apro il file: %s\n", files_list[index]);

    char * filepath = files_list[index];

    int charCount = getFileSize(filepath) - 1;

    if (charCount == 0)
        exit(0);

    int filePartsSize[4] = {};

    // TODO: cosa succede se charCount < 4 ?
    int baseSize = (charCount % 4 == 0) ? (charCount / 4) : (charCount / 4 + 1);
    filePartsSize[0] = filePartsSize[1] = filePartsSize[2] = baseSize;    
    filePartsSize[3] = charCount - baseSize * 3;

    int file_fd = open(filepath, O_RDONLY);
    if (file_fd == -1)
	    ErrExit("error while opening file");
    // Preparazione 4 messaggi

    char * fileParts[4] = {};
    for(int i = 0; i < 4; i++){
        printf("[DEBUG %d] Leggo n=%d caratteri\n", getpid(), filePartsSize[i]);
        fileParts[i] = (char *)calloc(filePartsSize[i], sizeof(char));
        read(file_fd, fileParts[i], sizeof(char) * filePartsSize[i]);
    }
    
    printf("[DEBUG %d] parte 1: %s\n", getpid(), fileParts[0]);
    printf("[DEBUG %d] parte 2: %s\n", getpid(), fileParts[1]);
    printf("[DEBUG %d] parte 3: %s\n", getpid(), fileParts[2]);
    printf("[DEBUG %d] parte 4: %s\n", getpid(), fileParts[3]);


    close(file_fd);
    exit(0);
}

void sigIntHandler(){
    printf("[DEBUG] signale SIGINT ricevuto: aggiorno la maschera dei segnali\n");

    // Aggiorno la maschera dei segnali (li comprende tutti)
    sigfillset(&signalSet);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    // apre la fifo
    fifo1_fd = open(FIFO1_NAME, O_WRONLY);

    // cambia directory di lavoro
    printf("[DEBUG] cambio cartella di lavoro\n");
    changeDir(workingDirectory);

    printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", getUsername(), workingDirectory); 

    // conta file con cui lavorare e salva i relativi filename nell'array
    N = getFiles(workingDirectory, files_list);

    printf("[DEBUG] Ho contato N=%d files\n", N);

    write(fifo1_fd, &N, sizeof(N));

    int shmid = alloc_shared_memory(SHM_KEY, sizeof(char)*50);
    char *shm_buffer = get_shared_memory(shmid, 0);

    int semid = semget(SEMAPHORE_KEY, 2, S_IRUSR | S_IWUSR);
    if(semid == -1)
        ErrExit("semget error");

    semOp(semid, (unsigned short)WAIT_DATA, -1);
    printf("[DEBUG] Leggo da Shared Memory: %s\n", shm_buffer);

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
    semOp(semid, (unsigned short)DATA_READY, 1);
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
