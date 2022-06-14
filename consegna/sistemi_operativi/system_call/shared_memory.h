/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

#include <sys/shm.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "defines.h"

#define SHM_KEY 5316
#define SHM_FLAGS_KEY 5317

static const int SHM_SIZE = (sizeof(message) * 50);
static const int SHM_FLAGS_SIZE = (sizeof(bool) * 50);

int alloc_shared_memory(key_t shmKey, size_t size);
void *get_shared_memory(int shmid, int shmflg);
void free_shared_memory(void *ptr_sh);
void remove_shared_memory(int shmid);