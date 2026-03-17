/*
 * קובץ: second_pass.h
 * מטרת הקובץ: ניהול והפעלת המעבר השני של האסמבלר.
 */
#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "globals.h"

/*
 * הפונקציה: run_second_pass
 * ------------------------
 * מבצעת את המעבר השני על קובץ האסמבלי (.am).
 * משלימה את קידוד הכתובות עבור אופרנדים מסוג תווית, מטפלת בהנחיות .entry,
 * ובסיום (אם לא התגלו שגיאות בשום שלב) קוראת לפונקציות יצירת קבצי הפלט.
 *
 * מחזירה: TRUE אם המעבר הסתיים ללא שגיאות, FALSE אחרת.
 */
boolean run_second_pass(const char *filename, AssemblerContext *context);

#endif
