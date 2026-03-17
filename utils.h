#ifndef UTILS_H
#define UTILS_H

#include "globals.h"
#include <stddef.h> /* עבור size_t */

/* פונקציות העזר הקיימות שלך */
void *safe_malloc(size_t size);
void skip_whitespaces(char **str);
boolean is_empty_or_comment(char *line);
char *create_file_name(const char *original_name, const char *extension);

/* --- פונקציות לניהול טבלת הסמלים --- */

/* הוספת סמל לטבלה. מחזירה FALSE אם הסמל כבר קיים */
boolean add_symbol(symbol_ptr *head, const char *name, int value, int is_code, int is_data, int is_extern);

/* חיפוש סמל בטבלה. מחזירה מצביע לסמל או NULL אם לא נמצא */
symbol_ptr get_symbol(symbol_ptr head, const char *name);

/* שחרור כל הזיכרון של טבלת הסמלים בסיום העבודה על קובץ */
void free_symbols(symbol_ptr head);

#endif
