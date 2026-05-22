#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <pthread.h>
#include <stdio.h>

// Globalny muteks chroniący zapis do pliku CSV (żeby logi się nie pomieszały)
extern pthread_mutex_t csv_mutex;

// Funkcja, którą będzie odpalał wątek roboczy
void process_url(const char* url, const char* base_domain, FILE* csv_file);

#endif