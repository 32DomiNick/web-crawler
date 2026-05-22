#ifndef PARSER_H
#define PARSER_H

// Funkcja analizuje kod HTML, wyciąga linki i od razu wrzuca je do kolejki
// Zwraca liczbę znalezionych linków, a wyciągnięty tytuł zapisuje do out_title
int parse_html_and_extract_links(const char* html_content, const char* base_domain, char* out_title, int title_max_len);

#endif // PARSER_H