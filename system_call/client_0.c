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

sigset_t signalSet;
char *homeDirectory;
char *workingDirectory;
int N;
int fifo1_fd;
char *files_list[100];

void child(int index){
    printf("[DEBUG] Sono il figlio: %d (index = %d)\n", getpid(), index);
    printf("[DEBUG] Apro il file: %s\n", files_list[index]);
    // FILE *file_fd = fopen("");
    exit(0);
}

void sigIntHandler(){
    printf("[DEBUG] signale SIGINT ricevuto: aggiorno la maschera dei segnali\n");
    sigfillset(&signalSet);
    sigprocmask(SIG_SETMASK, &signalSet, NULL);

    printf("[DEBUG] cambio cartella di lavoro\n");
    changeDir(workingDirectory);

    printf("Ciao %s, ora inizio l'invio dei file contenuti in %s\n", getUsername(), workingDirectory); 

    N = getFiles(workingDirectory, files_list);

    printf("[DEBUG] Ho contato N=%d files\n", N);

    fifo1_fd = open(FIFO1_PATHNAME, O_WRONLY);
    write(fifo1_fd, &N, sizeof(N));

    int shmid = alloc_shared_memory(SHM_KEY, sizeof(char)*50);
    char *shm_buffer = get_shared_memory(shmid, 0);

    int semid = semget(SEMAPHORE_KEY, 2, S_IRUSR | S_IWUSR);
    if(semid <= 0)
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
