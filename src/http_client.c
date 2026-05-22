#include "http_client.h"
#include "queue.h"
#include "visited.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>

pthread_mutex_t csv_mutex = PTHREAD_MUTEX_INITIALIZER;

// Struktura na pobrany kod HTML w pamięci operacyjnej
typedef struct {
    char* memory;
    size_t size;
} MemoryStruct;

// Callback dla curla - dokleja pobrane pakiety z sieci do naszego bufora
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) return 0; // Brak pamięci!

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

// Główna funkcja przetwarzająca URL
void process_url(const char* url, const char* base_domain, FILE* csv_file) {
    CURL *curl_handle = curl_easy_init();
    if(!curl_handle) return;

    MemoryStruct chunk;
    chunk.memory = malloc(1);  // Inicjalizacja bufora
    chunk.size = 0;

    // Konfiguracja curla: adres, funkcja zapisująca, podążanie za przekierowaniami (301, 302)
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Crawler-Wspolbiezny-Projekt/1.0");

    // Wykonanie żądania (pobieranie strony)
    CURLcode res = curl_easy_perform(curl_handle);

    if(res == CURLE_OK) {
        // --- PROSTY PARSER HTML ---
        char title[256] = "Brak tytulu";
        int link_count = 0;

        // 1. Szukamy tytułu: <title> ... </title>
        char* title_start = strstr(chunk.memory, "<title>");
        if(title_start) {
            title_start += 7; // Przesuwamy za "<title>"
            char* title_end = strstr(title_start, "</title>");
            if(title_end && (title_end - title_start < sizeof(title) - 1)) {
                strncpy(title, title_start, title_end - title_start);
                title[title_end - title_start] = '\0';
            }
        }

        // 2. Szukamy linków: href=" ... "
        char* search_ptr = chunk.memory;
        while ((search_ptr = strstr(search_ptr, "href=\"")) != NULL) {
            search_ptr += 6; // Przesuwamy za "href=\""
            char* link_end = strchr(search_ptr, '"');
            if (link_end) {
                link_count++;
                
                // Wyodrębniamy znaleziony link
                int link_len = link_end - search_ptr;
                char* new_link = strndup(search_ptr, link_len);

                // --- LOGIKA DODAWANIA DO KOLEJKI ---
                // Sprawdzamy czy link należy do naszej domeny bazowej
                if (strncmp(new_link, base_domain, strlen(base_domain)) == 0) {
                    // Sprawdzamy w hashmapie (odczyt), a następnie blokujemy do zapisu
                    if (!is_visited(new_link)) {
                        mark_visited(new_link);
                        push_url(new_link); // Wrzucamy do wspólnej kolejki!
                    }
                }
                free(new_link);
                search_ptr = link_end; // Szukamy dalej od tego miejsca
            }
        }

        // --- ZAPIS DO PLIKU CSV ---
        // Blokujemy plik muteksem, żeby wyniki z różnych wątków się nie wymieszały
        pthread_mutex_lock(&csv_mutex);
        fprintf(csv_file, "%s;%s;%d\n", url, title, link_count);
        fflush(csv_file); // Wymuszamy zapis na dysk
        pthread_mutex_unlock(&csv_mutex);
    }

    // Sprzątanie po curlu
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
}