/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <errno.h>

#include <string.h>

#include <signal.h>

#include "err_exit.h"


// Funzioni
char *getHomeDir();
char *getUsername();
void changeDir(char *dir);
bool beginswith(char *str, char *prefix);
void enumerate_dir(char * directory, int * count, char *files_list[]);
off_t getFileSize(char * pathname);
int findSHM(bool *shm, bool value);


// struct
typedef struct {
    char msg[1024];
    int pid;
    int index;
    char filename[150];
} message;

typedef struct {
    long mtype;
    message payload;
} msgqueue_message;