/*
 * קובץ: symbol_table.c
 * מימוש טבלת הסמלים באמצעות רשימה מקושרת דינאמית.
 */
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"
#include "utils.h"

symbol_ptr find_symbol(symbol_ptr head, const char *name) {
    symbol_ptr current = head;
    if (name == NULL) return NULL;
    
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

boolean add_symbol(symbol_ptr *head, const char *name, int value, 
                   boolean is_code, boolean is_data, boolean is_entry, boolean is_extern) {
    symbol_ptr new_node;
    
    /* חובה לוודא שהסמל לא הוגדר כבר (למעט מקרה של entry שיתווסף במעבר שני) */
    if (find_symbol(*head, name) != NULL) {
        return FALSE; 
    }
    
    new_node = (symbol_ptr)safe_malloc(sizeof(symbol_node));
    strncpy(new_node->name, name, MAX_LABEL_LENGTH);
    new_node->name[MAX_LABEL_LENGTH] = '\0';
    new_node->value = value;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_entry = is_entry;
    new_node->is_extern = is_extern;
    
    /* הוספה לראש הרשימה */
    new_node->next = *head;
    *head = new_node;
    
    return TRUE;
}

/* לפי עמוד 49 סעיף 19 בחוברת: יש לעדכן את ערכו של כל סמל נתונים ע"י הוספת ICF */
void update_data_symbols(symbol_ptr head, int icf) {
    symbol_ptr current = head;
    while (current != NULL) {
        if (current->is_data) {
            current->value += icf;
        }
        current = current->next;
    }
}

void free_symbol_table(symbol_ptr head) {
    symbol_ptr current = head;
    symbol_ptr next_node;
    while (current != NULL) {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}
