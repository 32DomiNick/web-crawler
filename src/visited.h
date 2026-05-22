#ifndef VISITED_H
#define VISITED_H

#include <pthread.h>
#include <stdbool.h>

// Globalna blokada odczytu/zapisu
extern pthread_rwlock_t visited_rwlock;

void init_visited();
bool mark_visited(const char* url);
bool is_visited(const char* url);
void destroy_visited();

#endif // VISITED_H