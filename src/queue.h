#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>

// Globalny muteks dostępny dla innych plików
extern pthread_mutex_t queue_mutex;

void init_queue();
bool push_url(const char* url);
char* pop_url();
void task_done(); // Ta funkcja jest kluczowa dla wiedzy o końcu zadań
void destroy_queue();

#endif // QUEUE_H