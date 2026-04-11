/*
 * קובץ: second_pass.h
 * מטרת הקובץ: אבות-טיפוס של המעבר השני וניהול רשימת סמלים חיצוניים.
 */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include <stdio.h>
#include "globals.h"

//בונים חוליה לרשימה שתכיל רק את התוויות שמוגדרטת extern שיהיה קל להדפיסם בקובץ הפלט שלהם. עושים זאת כי יש כתובות שלא ניתן לדעת ישר.
/* מבנה לשמירת כתובות של שימוש בסמלים חיצוניים (עבור קובץ .ext) */
typedef struct ext_node {
    char name[MAX_LABEL_LENGTH + 1];
    int address;
    struct ext_node *next;
} ext_node;

typedef ext_node *ext_ptr;

/*
 * הפונקציה: second_pass
 * ---------------------
 * מבצעת את המעבר השני. משלימה קידוד של תוויות ומעדכנת entry/extern.
 */
boolean second_pass(FILE *am_file, AssemblerContext *context, ext_ptr *ext_list_head);

#endif
