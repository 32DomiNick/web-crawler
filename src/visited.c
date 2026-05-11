#include "visited.h"

pthread_rwlock_t visited_rwlock = PTHREAD_RWLOCK_INITIALIZER;

void init_visited() {}
bool mark_visited(const char* url) { return false; }
bool is_visited(const char* url) { return false; }
void destroy_visited() {}