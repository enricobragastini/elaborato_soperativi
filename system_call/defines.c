/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
#include "err_exit.h"
#include <dirent.h>
#include <linux/limits.h>
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

bool beginswith(const char *str, const char *prefix){
    return strncmp(prefix, str, strlen(prefix)) == 0;
}

int getFileSize(char * pathname){
    struct stat stat_buffer;
    if(stat(pathname, &stat_buffer) == -1)
        ErrExit("getting file stats failed");
    
    off_t size = stat_buffer.st_size;
    return size;
}

void enumerate_dir(char * directory, int * count, char *files_list[]){
    DIR *dir;
    struct dirent *dentry;
    
    dir = opendir(directory);       // apro la cartella

    if(dir == NULL)
        ErrExit("error while opening directory");
    
    while((dentry = readdir(dir)) != NULL){
        // se trovo una cartella
        if(dentry->d_type == DT_DIR){
            if(strcmp(dentry->d_name, ".") != 0 && strcmp(dentry->d_name, "..") != 0){
                char * new_dir = (char *)calloc(strlen(directory) + strlen(dentry->d_name) + 1, sizeof(char));
                sprintf(new_dir, "%s/%s", directory, dentry->d_name);
                enumerate_dir(new_dir, count, files_list);              // ricerca ricorsiva
                free(new_dir);
            }
        }
        // se trovo un file (che ha un corretto filename)
        if(dentry->d_type == DT_REG && beginswith(dentry->d_name, "sendme_")){
            char filepath[strlen(directory) + strlen(dentry->d_name) + 1]; 
            sprintf(filepath, "%s/%s", directory, dentry->d_name);

            if(getFileSize(filepath) < 4096){
                // alloco spazio per il suo filename
                files_list[*count] = (char *)calloc(strlen(directory) + strlen(dentry->d_name) + 1, sizeof(char));
                strcpy(files_list[*count], filepath);   // salvo il suo filename
                (*count)++;                             // e aggiorno il conteggio
            }
        }
    }
    closedir(dir);
}