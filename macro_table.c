/*
 * קובץ: macro_table.c
 * מטרת הקובץ: ניהול טבלת המאקרואים ושורות הקוד שלהם בזיכרון.
 * כיצד מושגת המטרה: שימוש ברשימה מקושרת דינאמית של מאקרואים, 
 * שבה כל מאקרו מכיל רשימה מקושרת דינאמית פנימית של שורות התוכן שלו.
 */

#include <stdlib.h>
#include <string.h>
#include "macro_table.h"
#include "utils.h" 

/* מוסיף מאקרו חדש לרשימה */
macro_ptr add_macro(macro_ptr *head, const char *name) {
    macro_ptr new_macro;
    if (head == NULL || name == NULL) return NULL;
    
    new_macro = (macro_ptr)safe_malloc(sizeof(macro_node));
    strncpy(new_macro->name, name, MAX_LABEL_LENGTH);
    new_macro->name[MAX_LABEL_LENGTH] = '\0';
    new_macro->lines_head = NULL;
    new_macro->lines_tail = NULL;
    
    new_macro->next = *head;
    *head = new_macro;
    return new_macro;
}

/* מוסיף שורה לסוף המאקרו */
void add_macro_line(macro_ptr macro, const char *line) {
    macro_line_node *new_line;
    if (macro == NULL || line == NULL) return;
    
    new_line = (macro_line_node *)safe_malloc(sizeof(macro_line_node));
    strncpy(new_line->line, line, MAX_LINE_LENGTH + 1);
    new_line->line[MAX_LINE_LENGTH + 1] = '\0';
    new_line->next = NULL;
    
    if (macro->lines_head == NULL) {
        macro->lines_head = new_line;
        macro->lines_tail = new_line;
    } else {
        macro->lines_tail->next = new_line;
        macro->lines_tail = new_line;
    }
}

/* מחפש מאקרו לפי שם */
macro_ptr get_macro(macro_ptr head, const char *name) {
    macro_ptr current = head;
    if (name == NULL) return NULL;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}

/* משחרר את כל הזיכרון של הטבלה */
void free_macro_table(macro_ptr head) {
    macro_ptr current_macro = head;
    macro_ptr next_macro;
    macro_line_node *current_line;
    macro_line_node *next_line;
    
    while (current_macro != NULL) {
        current_line = current_macro->lines_head;
        while (current_line != NULL) {
            next_line = current_line->next;
            free(current_line);
            current_line = next_line;
        }
        next_macro = current_macro->next;
        free(current_macro);
        current_macro = next_macro;
    }
}
