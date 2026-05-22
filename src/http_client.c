#include "http_client.h"
#include "parser.h" // Zmiana: podpinamy nasz dedykowany moduł parsujący!
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t csv_mutex = PTHREAD_MUTEX_INITIALIZER;

// Struktura bufora na dane pobierane z sieci
typedef struct {
    char* memory;
    size_t size;
} MemoryStruct;

// Funkcja wywoływana przez libcurl przy odbieraniu paczek danych
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) return 0; 

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Null-terminator na końcu stringa

    return realsize;
}

void process_url(const char* url, const char* base_domain, FILE* csv_file) {
    CURL *curl_handle = curl_easy_init();
    if(!curl_handle) return;

    MemoryStruct chunk;
    chunk.memory = malloc(1);  
    chunk.size = 0;

    // Konfiguracja CURL-a
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L); // Obsługa kodów 301/302
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Crawler-Wspolbiezny-Projekt/1.0");

    // Pobranie kodu HTML
    CURLcode res = curl_easy_perform(curl_handle);

    if(res == CURLE_OK) {
        char title[256];
        
        // Zlecamy całą brudną robotę z tekstem naszemu modułowi parser.c
        int link_count = parse_html_and_extract_links(chunk.memory, base_domain, title, sizeof(title));

        // Zapis do CSV chroniony muteksem
        pthread_mutex_lock(&csv_mutex);
        fprintf(csv_file, "%s;%s;%d\n", url, title, link_count);
        fflush(csv_file); 
        pthread_mutex_unlock(&csv_mutex);
    }

    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
}