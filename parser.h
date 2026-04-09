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
// צריך להוסיף אותה בקוד שלנו ובנוסף- להוסיף גם עוד מספר פונקציות כדי להקל על pre assembler (בשיחה עם הגימניי- לבדוק)
//נצטרך גם להכניס פונקציה שתעבוד עם short כפי שג'ודי ביקשה כי כרגע הם לא על short
// יש שורה לא קריאה בfirst pass שהגימנאיי ממליץ במקומה לעשות פה פונקציה נוספת שתקל על הכתיבה שם. (לבדוק)
boolean is_valid_addr_mode(const int modes[], int mode);


//----------------------------------------------------------------------------------------------------------------------
// פונקציות שלא כתובות בכלל בפרויקט ואין להם מימוש! מומלץ לכתוב אותם ולהקל עלינו בfirst pass במקום מלא קוד-
// כי כרגע במקום לממש אותם כתוב שם הכל בתנאים ובבלוק גדול.

/* מנתחת הנחיית .string (קוראת מחרוזת בתוך מרכאות, מכניסה למערך הנתונים ומוסיפה 0 בסוף) */
boolean parse_string_directive(char **line_ptr, AssemblerContext *ctx);

/* מנתחת הנחיית .data (קוראת מספרים שלמים מופרדים בפסיקים ומכניסה למערך הנתונים) */
boolean parse_data_directive(char **line_ptr, AssemblerContext *ctx);

#endif

