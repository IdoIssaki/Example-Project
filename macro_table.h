/*
 * קובץ: macro_table.h
 * מטרת הקובץ: הגדרת מבני הנתונים ואבות-הטיפוס לניהול טבלת המאקרואים.
 */

#ifndef MACRO_TABLE_H
#define MACRO_TABLE_H

#include "globals.h"

/* צומת המייצג שורה בודדת בתוך גוף המאקרו */
typedef struct macro_line_node {
    char line[MAX_LINE_LENGTH + 2]; /* מקום לתוכן השורה, תווים מיוחדים וסיום מחרוזת */
    struct macro_line_node *next;
} macro_line_node;

/* צומת המייצג מאקרו שלם ברשימה המקושרת של טבלת המאקרואים */
typedef struct macro_node {
    char name[MAX_LABEL_LENGTH + 1]; /* שם המאקרו */
    macro_line_node *lines_head;     /* מצביע לשורה הראשונה בתוכן המאקרו */
    macro_line_node *lines_tail;     /* מצביע לשורה האחרונה (לייעול הוספת שורות) */
    struct macro_node *next;         /* מצביע למאקרו הבא בטבלה */
} macro_node;

typedef macro_node *macro_ptr;

/*
 * הפונקציה: add_macro
 * -----------------
 * קלט: head - מצביע למצביע של ראש טבלת המאקרואים. name - שם המאקרו החדש.
 * פלט: מצביע למאקרו החדש שנוצר.
 * תיאור: מקצה צומת חדש עבור המאקרו, מאתחלת אותו, ומוסיפה אותו לראש הרשימה.
 */
macro_ptr add_macro(macro_ptr *head, const char *name);

/*
 * הפונקציה: add_macro_line
 * ------------------------
 * קלט: macro - מצביע למאקרו שאליו מוסיפים. line - מחרוזת השורה להוספה.
 * פלט: אין.
 * תיאור: יוצרת צומת שורה חדש, מעתיקה אליו את תוכן השורה, ומוסיפה לסוף הרשימה
 * הפנימית של המאקרו הנתון.
 */
void add_macro_line(macro_ptr macro, const char *line);

/*
 * הפונקציה: get_macro
 * -------------------
 * קלט: head - מצביע לראש טבלת המאקרואים. name - שם המאקרו לחיפוש.
 * פלט: מצביע למאקרו אם נמצא, או NULL אם אינו בטבלה.
 * תיאור: סורקת את הטבלה ומחפשת מאקרו לפי שמו.
 */
macro_ptr get_macro(macro_ptr head, const char *name);

/*
 * הפונקציה: free_macro_table
 * --------------------------
 * קלט: head - מצביע לראש טבלת המאקרואים.
 * פלט: אין.
 * תיאור: משחררת באופן בטוח את כל הזיכרון הדינאמי שהוקצה לטבלה ולשורות הפנימיות.
 */
void free_macro_table(macro_ptr head);

#endif
