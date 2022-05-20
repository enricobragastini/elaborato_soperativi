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

#include <string.h>

#include <signal.h>

#include "err_exit.h"


// Funzioni
char *getHomeDir();
char *getUsername();
void changeDir(char *dir);
bool beginswith(const char *str, const char *prefix);
int getFiles(char * wd, char ** files_list);
int getFileSize(char * pathname);