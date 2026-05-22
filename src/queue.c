#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Struktura węzła dla prostej listy jednokierunkowej
typedef struct Node {
    char* url;
    struct Node* next;
} Node;

// Globalne wskaźniki na początek i koniec kolejki
static Node* head = NULL;
static Node* tail = NULL;

// Inicjalizacja blokad i zmiennych warunkowych
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

// Zmienne do kontrolowania stanu zakończenia pracy crawlera
static int active_tasks = 0;   // Ile wątków aktualnie przetwarza stronę
static bool is_finished = false; // Czy program ma się zakończyć

// Dodawanie nowego URL do kolejki
bool push_url(const char* url) {
    // 1. Alokacja pamięci dla nowego węzła i skopiowanie adresu URL
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (!new_node) return false;
    new_node->url = strdup(url); // Klonujemy string
    new_node->next = NULL;

    // 2. Blokujemy muteks, bo będziemy modyfikować wspólną kolejkę
    pthread_mutex_lock(&queue_mutex);

    // 3. Dodajemy element na koniec kolejki (FIFO)
    if (tail == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }

    // 4. Budzimy JEDEN ze śpiących wątków, informując go: "Hej, jest nowa robota!"
    pthread_cond_signal(&queue_cond);
    
    // 5. Zwalniamy blokadę
    pthread_mutex_unlock(&queue_mutex);
    return true;
}

// Pobieranie URL z kolejki (wątek może tu zasnąć, jeśli kolejka jest pusta)
char* pop_url() {
    pthread_mutex_lock(&queue_mutex);

    // Dopóki kolejka jest pusta i program wciąż działa, wątek śpi (czeka na sygnał)
    while (head == NULL && !is_finished) {
        pthread_cond_wait(&queue_cond, &queue_mutex);
    }

    // Jeśli obudzono nas, a program ma się zakończyć, wychodzimy
    if (is_finished && head == NULL) {
        pthread_mutex_unlock(&queue_mutex);
        return NULL;
    }

    // Ściągamy element z przodu kolejki
    Node* temp = head;
    char* url = temp->url;
    head = head->next;
    if (head == NULL) {
        tail = NULL;
    }
    
    free(temp); // Zwalniamy sam węzeł, string (url) oddajemy do wątku

    // Zaznaczamy, że wątek wziął zadanie i jest "aktywny"
    active_tasks++;

    pthread_mutex_unlock(&queue_mutex);
    return url;
}

// Funkcja wywoływana przez wątek, gdy skończy przetwarzać pobrany URL
void task_done() {
    pthread_mutex_lock(&queue_mutex);
    active_tasks--; // Zmniejszamy licznik aktywnych wątków

    // Jeśli kolejka jest pusta i nikt już nic nie przetwarza -> to koniec internetu! (przynajmniej w naszej domenie)
    if (head == NULL && active_tasks == 0) {
        is_finished = true;
        // Budzimy WSZYSTKIE uśpione wątki, żeby mogły bezpiecznie zakończyć swoje działanie (return NULL)
        pthread_cond_broadcast(&queue_cond);
    }
    pthread_mutex_unlock(&queue_mutex);
}

// Resetowanie kolejki przed każdym nowym pomiarem czasowym
void init_queue() {
    head = NULL;
    tail = NULL;
    active_tasks = 0;
    is_finished = false;
}

void destroy_queue() {
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_cond);
    
    // Zwalnianie ewentualnych resztek z kolejki
    Node* current = head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp->url);
        free(temp);
    }
}