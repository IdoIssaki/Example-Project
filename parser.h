/*
 * קובץ: parser.h
 */
#ifndef PARSER_H
#define PARSER_H

#include "globals.h"

boolean is_reserved_word(const char *word);
boolean is_valid_label_name(const char *name);
void extract_word(char **source, char *dest);
boolean is_label_definition(const char *word);
boolean is_valid_number(const char *str);
boolean is_valid_string_literal(const char *str);

#endif

