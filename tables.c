/*
 * קובץ: tables.c
 * מכיל את טבלת הפקודות הקבועות לפי עמוד 46 בחוברת הפרויקט.
 * וכן פונקציות עזר לזיהוי פקודות ורגיסטרים.
 */
#include <string.h>
#include "tables.h"

#define NUM_REGISTERS 8

/* * טבלת פקודות האסמבלי:
 * המבנה: {שם, opcode, funct, מספר אופרנדים, {חוקיות מקור 0,1,2,3}, {חוקיות יעד 0,1,2,3}} 
 */
static const command_entry commands_table[NUM_COMMANDS] = {
    {"mov",  0,  0, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"cmp",  1,  0, 2, {1, 1, 0, 1}, {1, 1, 0, 1}},
    {"add",  2, 10, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"sub",  2, 11, 2, {1, 1, 0, 1}, {0, 1, 0, 1}},
    {"lea",  4,  0, 2, {0, 1, 0, 0}, {0, 1, 0, 1}},
    {"clr",  5, 10, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"not",  5, 11, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"inc",  5, 12, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"dec",  5, 13, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"jmp",  9, 10, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"bne",  9, 11, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"jsr",  9, 12, 1, {0, 0, 0, 0}, {0, 1, 1, 0}},
    {"red", 12,  0, 1, {0, 0, 0, 0}, {0, 1, 0, 1}},
    {"prn", 13,  0, 1, {0, 0, 0, 0}, {1, 1, 0, 1}},
    {"rts", 14,  0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {"stop", 15, 0, 0, {0, 0, 0, 0}, {0, 0, 0, 0}}
};

/* מערך של כל הרגיסטרים החוקיים */
static const char *registers_table[NUM_REGISTERS] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
};

const command_entry *get_command(const char *name) {
    int i;
    if (name == NULL) return NULL;
    for (i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(commands_table[i].name, name) == 0) {
            return &commands_table[i];
        }
    }
    return NULL;
}

boolean is_register(const char *name) {
    int i;
    if (name == NULL) return FALSE;
    for (i = 0; i < NUM_REGISTERS; i++) {
        if (strcmp(registers_table[i], name) == 0) return TRUE;
    }
    return FALSE;
}
