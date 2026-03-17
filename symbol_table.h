/*
 * קובץ: symbol_table.h
 * מטרת הקובץ: ניהול טבלת הסמלים (תוויות) של האסמבלר.
 */
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "globals.h"

/* מחפשת סמל בטבלה לפי שמו. מחזירה מצביע לצומת אם נמצא, או NULL אם לא. */
symbol_ptr find_symbol(symbol_ptr head, const char *name);

/* מוסיפה סמל חדש לטבלה. מחזירה TRUE בהצלחה, או FALSE אם הסמל כבר קיים. */
boolean add_symbol(symbol_ptr *head, const char *name, int value, 
                   boolean is_code, boolean is_data, boolean is_entry, boolean is_extern);

/* עדכון כתובות סמלי הנתונים בסוף המעבר הראשון (לפי שלב 19 באלגוריתם בעמוד 49) */
void update_data_symbols(symbol_ptr head, int icf);

/* שחרור הזיכרון של כל טבלת הסמלים (בסיום התוכנית) */
void free_symbol_table(symbol_ptr head);

#endif
