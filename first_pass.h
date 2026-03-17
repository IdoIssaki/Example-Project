/*
 * קובץ: first_pass.h
 * מטרת הקובץ: אבות-טיפוס של המעבר הראשון (First Pass).
 */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include <stdio.h>
#include "globals.h"

/*
 * הפונקציה: first_pass
 * --------------------
 * קלט: am_file - מצביע לקובץ ה-.am (אחרי פרישת מאקרו).
 * context - מבנה ההקשר של האסמבלר (מונים, תמונת זיכרון, טבלת סמלים).
 * פלט: TRUE אם המעבר עבר בהצלחה ללא שגיאות, אחרת FALSE.
 */
boolean first_pass(FILE *am_file, AssemblerContext *context);

#endif