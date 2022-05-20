/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

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

int getFiles(char * wd, char *files_list[]){
    // DA FARE: ricerca ricorsiva in sottocartelle

    int count = 0;

    DIR *dp = opendir(wd);
    if (dp == NULL)
        ErrExit("Couldn't open given folder");

    int errno = 0;
    struct dirent *dentry;
    // Iterate until NULL is returned as a result.
    while((dentry = readdir(dp)) != NULL){
        if(dentry->d_type == DT_REG && beginswith(dentry->d_name, "sendme_") && getFileSize(dentry->d_name) < 4096){
            files_list[count] = dentry->d_name;
            count++;
        }
        errno = 0;
    }

    // NULL is returned on error, and when the end-of-directory is reached!
    if (errno != 0)
        ErrExit("Error while reading dir.");
    closedir(dp);

    return count;
}