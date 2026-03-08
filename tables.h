/*
 * קובץ: tables.h
 */
#ifndef TABLES_H
#define TABLES_H

#include "globals.h"

const command_entry *get_command(const char *name);
boolean is_register(const char *name);

#endif
