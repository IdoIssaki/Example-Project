/*
 * קובץ: tables.c
 */
#include <string.h>
#include "tables.h"

#define NUM_REGISTERS 8

static const command_entry commands_table[NUM_COMMANDS] = {
    {"mov", 0, 0}, {"cmp", 1, 0}, {"add", 2, 10}, {"sub", 2, 11},
    {"lea", 4, 0}, {"clr", 5, 10}, {"not", 5, 11}, {"inc", 5, 12},
    {"dec", 5, 13}, {"jmp", 9, 10}, {"bne", 9, 11}, {"jsr", 9, 12},
    {"red", 12, 0}, {"prn", 13, 0}, {"rts", 14, 0}, {"stop", 15, 0}
};

static const char *registers_table[NUM_REGISTERS] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"
};

const command_entry *get_command(const char *name) {
    int i;
    if (name == NULL) return NULL;
    for (i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(commands_table[i].name, name) == 0) return &commands_table[i];
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
