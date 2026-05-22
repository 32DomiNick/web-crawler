#include "visited.h"
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 10007 // Duża liczba pierwsza dla lepszego rozkładu haszowania

// Węzeł dla tablicy haszującej (obsługa kolizji przez łączenie w listę)
typedef struct VNode {
    char* url;
    struct VNode* next;
} VNode;

static VNode* hash_table[HASH_SIZE];

// Blokada Odczytu-Zapisu (Wiele wątków może czytać, tylko jeden zapisywać)
pthread_rwlock_t visited_rwlock = PTHREAD_RWLOCK_INITIALIZER;

// Prosta funkcja haszująca (algorytm djb2) zamieniająca string na liczbę indeksu
static unsigned int hash_str(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_SIZE;
}

// Sprawdzenie, czy URL był już odwiedzony (Blokada ODCZYTU)
bool is_visited(const char* url) {
    unsigned int index = hash_str(url);
    
    // ZAKŁADAMY BLOKADĘ TYLKO DO ODCZYTU! Wiele wątków może to robić jednocześnie.
    pthread_rwlock_rdlock(&visited_rwlock);
    
    VNode* current = hash_table[index];
    while (current != NULL) {
        if (strcmp(current->url, url) == 0) {
            pthread_rwlock_unlock(&visited_rwlock);
            return true; // Znaleziono, już tu byliśmy!
        }
        current = current->next;
    }
    
    pthread_rwlock_unlock(&visited_rwlock);
    return false; // Nie znaleziono
}

// Oznaczanie adresu jako odwiedzonego (Blokada ZAPISU)
bool mark_visited(const char* url) {
    // Jeśli już odwiedzony, nie robimy nic
    if (is_visited(url)) return false; 

    unsigned int index = hash_str(url);
    VNode* new_node = (VNode*)malloc(sizeof(VNode));
    new_node->url = strdup(url);
    
    // ZAKŁADAMY BLOKADĘ DO ZAPISU! (Wszystkie inne wątki muszą czekać)
    pthread_rwlock_wrlock(&visited_rwlock);
    
    // Wstawiamy na początek listy pod danym indeksem
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
    
    pthread_rwlock_unlock(&visited_rwlock);
    return true;
}

// Resetowanie tablicy przed kolejnym testem wydajnościowym
void init_visited() {
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_table[i] = NULL;
    }
}