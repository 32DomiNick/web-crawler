#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>

// Deklaracja mutexa jako zewnętrznej zmiennej, aby main.c mógł go zainicjować/zniszczyć
extern pthread_mutex_t queue_mutex;

// Tutaj w przyszłości dodamy strukturę kolejki
void init_queue();
bool push_url(const char* url);
char* pop_url();
void destroy_queue();

#endif // QUEUE_H