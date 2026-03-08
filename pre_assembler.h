/*
 * קובץ: pre_assembler.h
 * מטרת הקובץ: אבות-טיפוס לפונקציות המנהלות את שלב קדם-האסמבלר.
 */

#ifndef PRE_ASSEMBLER_H
#define PRE_ASSEMBLER_H

#include <stdio.h>
#include "globals.h"

/*
 * הפונקציה: pre_assemble
 * ----------------------
 * קלט: source_file - מצביע לקובץ המקור. base_file_name - שם הקובץ (ללא סיומת).
 * context - מצביע למבנה ההקשר (לטיפול בשגיאות).
 * פלט: TRUE אם עבר בהצלחה, FALSE אם נמצאו שגיאות.
 * תיאור: פורשת מאקרואים ויוצרת את קובץ ה- .am.
 */
boolean pre_assemble(FILE *source_file, const char *base_file_name, AssemblerContext *context);

#endif
