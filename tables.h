/*
 * קובץ: tables.h
 * חשיפת פונקציות העזר הקשורות לטבלאות הקבועות של האסמבלר.
 */
#ifndef TABLES_H
#define TABLES_H

#include "globals.h"

/* מחפשת פקודה לפי שם. מחזירה מצביע למבנה הנתונים, או NULL אם לא נמצאה. */
const command_entry *get_command(const char *name);

/* בודקת האם מחרוזת נתונה היא שם חוקי של רגיסטר (r0-r7) */
boolean is_register(const char *name);

#endif
