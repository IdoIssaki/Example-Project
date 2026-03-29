/*
 * קובץ: parser.h
 */
#ifndef PARSER_H
#define PARSER_H

#include "globals.h"

boolean is_reserved_word(const char *word);
boolean is_valid_label_name(const char *name);
void extract_word(char **source, char *dest);
boolean is_label_definition(const char *word);
boolean is_valid_number(const char *str);
boolean is_valid_string_literal(const char *str);
boolean is_valid_addr_mode(const int modes[], int mode);

/* מנתחת הנחיית .string (קוראת מחרוזת בתוך מרכאות, מכניסה למערך הנתונים ומוסיפה 0 בסוף) */
boolean parse_string_directive(char **line_ptr, AssemblerContext *ctx);

/* מנתחת הנחיית .data (קוראת מספרים שלמים מופרדים בפסיקים ומכניסה למערך הנתונים) */
boolean parse_data_directive(char **line_ptr, AssemblerContext *ctx);



#endif

