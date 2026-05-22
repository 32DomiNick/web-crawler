#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>
#include <time.h>
#include "queue.h"
#include "visited.h"
#include "http_client.h"

// Ograniczamy crawlera tylko do tej domeny 
const char* BASE_DOMAIN = "http://example.com"; 

FILE* output_csv;
int total_pages_processed = 0; // Do pomiarów wydajności

// Struktura przekazywana do wątków
typedef struct {
    int thread_id;
} ThreadData;

// --- GŁÓWNA FUNKCJA WĄTKU ---
void* worker_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    while (1) {
        // Pobierz zadanie z kolejki (tutaj wątek śpi, jeśli nie ma co robić)
        char* current_url = pop_url();
        
        // Jeśli obudził się z NULLem, oznacza to koniec pracy crawlera
        if (current_url == NULL) {
            break;
        }

        // Przetwarza stronę (Curl, Parser, CSV)
        process_url(current_url, BASE_DOMAIN, output_csv);
        
        // Bezpieczne zwiększenie licznika stron za pomocą atomowej operacji GCC
        __sync_fetch_and_add(&total_pages_processed, 1);

        free(current_url); // Zwalniamy pamięć stringa po przetworzeniu
        
        // Zgłaszamy kolejce, że zadanie wykonane (obniża aktywny licznik)
        task_done(); 
    }

    return NULL;
}

// Funkcja testująca dla konkretnej puli wątków
void run_crawler_test(int num_threads) {
    printf("\n=== Uruchamianie testu dla %d watkow ===\n", num_threads);
    
    // Zerowanie danych i struktur
    init_queue();
    init_visited();
    total_pages_processed = 0;

    // Inicjowanie stanu początkowego (Seed)
    mark_visited(BASE_DOMAIN);
    push_url(BASE_DOMAIN);

    pthread_t threads[16]; // Max 16 wg specyfikacji
    ThreadData t_data[16];

    // Rozpoczęcie pomiaru czasu (Zgodnie z wymaganiami z PDF POSIX)
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Odpalanie wątków
    for (int i = 0; i < num_threads; i++) {
        t_data[i].thread_id = i + 1;
        pthread_create(&threads[i], NULL, worker_thread, &t_data[i]);
    }

    // Oczekiwanie na dołączenie (zakonczenie)
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Koniec pomiaru
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

    // Obliczanie wydajności (strony na sekunde)
    double pages_per_sec = total_pages_processed / elapsed_time;

    printf("Koniec testu (%d watkow).\n", num_threads);
    printf("Przetworzono stron: %d\n", total_pages_processed);
    printf("Calkowity czas: %.2f sekund\n", elapsed_time);
    printf("Wydajnosc: %.2f stron/s\n", pages_per_sec);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    output_csv = fopen("wyniki_crawlowania.csv", "w");
    if (!output_csv) {
        printf("Blad otwarcia pliku CSV!\n");
        return 1;
    }
    // Nagłówek pliku
    fprintf(output_csv, "URL;Tytul;Liczba linkow\n");

    // Przeprowadzamy 4 testy
    int thread_counts[] = {1, 4, 8, 16};
    
    for (int i = 0; i < 4; i++) {
        run_crawler_test(thread_counts[i]);
    }

    fclose(output_csv);
    curl_global_cleanup();
    printf("\nTesty wydajnosciowe zakonczone! Zobacz wyniki_crawlowania.csv\n");
    return 0;
}