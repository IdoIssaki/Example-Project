/*
 * קובץ: utils.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Fatal Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void skip_whitespaces(char **str) {
    if (str == NULL || *str == NULL) return;
    while (**str == ' ' || **str == '\t') {
        (*str)++;
    }
}

boolean is_empty_or_comment(char *line) {
    char *ptr = line;
    if (line == NULL) return TRUE;
    skip_whitespaces(&ptr);
    if (*ptr == '\0' || *ptr == '\n' || *ptr == ';') return TRUE;
    return FALSE;
}

char *create_file_name(const char *original_name, const char *extension) {
    char *full_name;
    size_t length;
    if (original_name == NULL || extension == NULL) return NULL;
    length = strlen(original_name) + strlen(extension) + 1;
    full_name = (char *)safe_malloc(length);
    strcpy(full_name, original_name);
    strcat(full_name, extension);
    return full_name;
}
