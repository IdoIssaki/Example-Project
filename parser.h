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


/* בודק אם שיטת מיעון חוקית לפקודה */
// צריך להוסיף אותה בקוד שלנו
boolean is_valid_addr_mode(const int modes[], int mode);


//----------------------------------------------------------------------------------------------------------------------
// פונקציות שלא כתובות בכלל בפרויקט ואין להם מימוש! מומלץ לכתוב אותם ולהקל עלינו בfirst pass במקום מלא קוד-
// כי כרגע במקום לממש אותם כתוב שם הכל בתנאים ובבלוק גדול.

//זו ספציפית לא חוסכת קוד אלא הופכת קוד לקריא יותר
/* מזהה את שיטת המיעון של אופרנד (0, 1, 2, או 3) */
int get_addressing_mode(const char *operand);

/* מנתחת הנחיית .string (קוראת מחרוזת בתוך מרכאות, מכניסה למערך הנתונים ומוסיפה 0 בסוף) */
boolean parse_string_directive(char **line_ptr, AssemblerContext *ctx);

/* מנתחת הנחיית .data (קוראת מספרים שלמים מופרדים בפסיקים ומכניסה למערך הנתונים) */
boolean parse_data_directive(char **line_ptr, AssemblerContext *ctx);

/* מנתחת אופרנדים של פקודה (מפרידה בין מקור ליעד, בודקת פסיקים ותקינות) */
boolean parse_command_operands(char **line_ptr, char *src, char *dst, int expected_ops, int line_number);

#endif

