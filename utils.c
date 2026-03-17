/*
 * קובץ: utils.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/* --- פונקציות העזר המקוריות שלך --- */

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

/* --- פונקציות ניהול טבלת הסמלים (חדש) --- */

boolean add_symbol(symbol_ptr *head, const char *name, int value, int is_code, int is_data, int is_extern) {
    symbol_ptr new_node;
    symbol_ptr current = *head;

    /* בדיקה אם הסמל כבר קיים בטבלה (מונע כפילויות) */
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return FALSE;
        }
        current = current->next;
    }

    /* שימוש בפונקציה שלך להקצאה בטוחה! */
    new_node = (symbol_ptr)safe_malloc(sizeof(symbol_node));

    strcpy(new_node->name, name);
    new_node->value = value;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_extern = is_extern;
    new_node->is_entry = FALSE; /* מתעדכן במעבר השני אם צריך */

    /* הוספת הסמל לראש הרשימה */
    new_node->next = *head;
    *head = new_node;

    return TRUE;
}

symbol_ptr get_symbol(symbol_ptr head, const char *name) {
    symbol_ptr current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current; /* נמצא */
        }
        current = current->next;
    }
    return NULL; /* לא נמצא */
}

void free_symbols(symbol_ptr head) {
    symbol_ptr temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp); /* שחרור הזיכרון של כל צומת */
    }
}
