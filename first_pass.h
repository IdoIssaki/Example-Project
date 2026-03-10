/*
 * File: first_pass.h
 * Purpose: Manages the First Pass of the assembler.
 */
#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "globals.h"

/*
 * Executes the First Pass on the given .am file.
 * Builds the symbol table, updates IC/DC, and validates syntax.
 * * Returns TRUE if the pass completed without errors, FALSE otherwise.
 */
boolean execute_first_pass(const char *am_file_name, AssemblerContext *context);

#endif
