/*
<<<<<<< HEAD
 * קובץ: first_pass.h
 * מטרת הקובץ: ניהול והפעלת המעבר הראשון של האסמבלר.
=======
 * File: first_pass.h
 * Purpose: Manages the First Pass of the assembler.
>>>>>>> V1
 */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "globals.h"

/*
<<<<<<< HEAD
 * הפונקציה: run_first_pass
 * ------------------------
 * מבצעת את המעבר הראשון על קובץ האסמבלי לאחר פרישת מאקרו (סיומת .am).
 * האלגוריתם עוקב אחר ההנחיות בעמוד 48-49 בחוברת הפרויקט:
 * 1. מזהה תוויות ומוסיף לטבלת הסמלים.
 * 2. מקודד הנחיות נתונים (.data, .string) לתמונת הנתונים.
 * 3. מחשב את גודל ההוראות ומקודד את מילות הבסיס לתמונת ההוראות.
 * * מחזירה: TRUE אם המעבר הסתיים ללא שגיאות, FALSE אחרת.
 */
boolean run_first_pass(const char *filename, AssemblerContext *context);
=======
 * Executes the First Pass on the given .am file.
 * Builds the symbol table, updates IC/DC, and validates syntax.
 * * Returns TRUE if the pass completed without errors, FALSE otherwise.
 */
boolean execute_first_pass(const char *am_file_name, AssemblerContext *context);
>>>>>>> V1

#endif
