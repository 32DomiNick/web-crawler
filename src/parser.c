#include "parser.h"
#include "queue.h"
#include "visited.h"
#include <string.h>
#include <stdlib.h>

int parse_html_and_extract_links(const char* html_content, const char* base_domain, char* out_title, int title_max_len) {
    int link_count = 0;
    
    // Zabezpieczenie, gdyby strona nie miała tytułu
    strncpy(out_title, "Brak tytulu", title_max_len);

    // 1. Wyciąganie tytułu strony: <title> ... </title>
    const char* title_start = strstr(html_content, "<title>");
    if (title_start) {
        title_start += 7; // Przesuwamy wskaźnik za tag "<title>"
        const char* title_end = strstr(title_start, "</title>");
        if (title_end && (title_end - title_start < title_max_len - 1)) {
            strncpy(out_title, title_start, title_end - title_start);
            out_title[title_end - title_start] = '\0';
        }
    }

    // 2. Wyciąganie linków i sprawdzanie domeny
    const char* search_ptr = html_content;
    while ((search_ptr = strstr(search_ptr, "href=\"")) != NULL) {
        search_ptr += 6; // Przesuwamy za "href=\""
        const char* link_end = strchr(search_ptr, '"');
        
        if (link_end) {
            link_count++;
            
            int link_len = link_end - search_ptr;
            char* new_link = (char*)malloc(link_len + 1);
            
            if (new_link != NULL) {
                strncpy(new_link, search_ptr, link_len);
                new_link[link_len] = '\0';

                char* final_link = NULL;

                // Przypadek 1: Link absolutny (np. http://quotes.toscrape.com/author/Einstein)
                if (strncmp(new_link, base_domain, strlen(base_domain)) == 0) {
                    final_link = strdup(new_link);
                }
                // Przypadek 2: Link względny (np. /page/2/)
                else if (new_link[0] == '/') {
                    final_link = (char*)malloc(strlen(base_domain) + strlen(new_link) + 1);
                    strcpy(final_link, base_domain);
                    strcat(final_link, new_link);
                }

                // Jeśli pomyślnie stworzyliśmy pełen adres
                if (final_link) {
                    // Blokada ODCZYTU (is_visited) i ZAPISU (mark_visited)
                    if (!is_visited(final_link)) {
                        mark_visited(final_link);
                        push_url(final_link); // Wrzucamy do kolejki
                    }
                    free(final_link);
                }
                free(new_link); 
            }
            search_ptr = link_end; // Przesuwamy się dalej, by szukać kolejnych
        }
    }
    return link_count;
}