#include "queue.h"

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_queue() {}
bool push_url(const char* url) { return false; }
char* pop_url() { return NULL; }
void destroy_queue() {}