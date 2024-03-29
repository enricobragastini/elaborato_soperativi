/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include "err_exit.h"
#include <dirent.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *getHomeDir(){
    char *homedir = getenv("HOME");             
    if(homedir == NULL)
        ErrExit("getting home directory failed");
    strcat(homedir, "/");
    return homedir;
}

char *getUsername(){
    char *username = getlogin();
    if (username == NULL)
        ErrExit("username couldn't be determined");
    return username;
}

void changeDir(char *dir){
    if(chdir(dir) == -1)
        ErrExit("chdir failed");

    char cwd[PATH_MAX];                         
    if (getcwd(cwd, PATH_MAX) == NULL)
        ErrExit("getcwd failed");
    
    strcpy(dir, cwd);
}

bool hasValidFilename(char *str){
    char * prefix = "sendme_";
    bool hasValidPrefix, hasValidExtension;

    hasValidPrefix = strncmp(prefix, str, strlen(prefix)) == 0;
    
    char * point;
    if((point = strrchr(str,'.')) != NULL )
        hasValidExtension = strcmp(point,".txt") == 0;

    return hasValidPrefix && hasValidExtension;
}

bool contains(char *str, char *substr){
    return strstr(str, substr) != NULL;
}

off_t getFileSize(char * pathname){
    struct stat stat_buffer;
    if(stat(pathname, &stat_buffer) == -1){
        printf("stat error: %s\n", pathname);
        ErrExit("getting file stats failed");
    }
    return stat_buffer.st_size;
}

void enumerate_dir(char * directory, int * count, char *files_list[]){
    DIR *dir;
    struct dirent *dentry;
    
    dir = opendir(directory);       // apro la cartella

    *(count) = 0;

    if(dir == NULL)
        ErrExit("error while opening directory");
    
    while((dentry = readdir(dir)) != NULL && *(count) < 100){
        // se trovo una cartella
        if(dentry->d_type == DT_DIR){
            if(strcmp(dentry->d_name, ".") != 0 && strcmp(dentry->d_name, "..") != 0){
                char * new_dir = (char *)calloc(PATH_MAX, sizeof(char));
                snprintf(new_dir, PATH_MAX, "%s/%s", directory, dentry->d_name);
                enumerate_dir(new_dir, count, files_list);              // ricerca ricorsiva
                free(new_dir);
            }
        }
        // se trovo un file (che ha un corretto filename)
        if(dentry->d_type == DT_REG && hasValidFilename(dentry->d_name) && !contains(dentry->d_name, "_out")){
            char filepath[PATH_MAX]; 
            snprintf(filepath,PATH_MAX, "%s/%s", directory, dentry->d_name);

            size_t charCount = getFileSize(filepath);
            if(charCount < 4096 && !(charCount < 4 || (charCount > 4 && charCount < 7) || charCount == 9)){
                // alloco spazio per il suo filename
                files_list[*count] = (char *)calloc(PATH_MAX, sizeof(char));
                strcpy(files_list[*count], filepath);   // salvo il suo filename
                (*count)++;                             // e aggiorno il conteggio
            }
        }
    }
    closedir(dir);
}

int findSHM(bool *shm, bool value){
    // Cerca posizione libera (o occupata)
    // nell'array di supporto alla shared memory

    for(int i = 0; i<50; i++){
        if(shm[i] == value)
            return i;
    }
    return -1;
}