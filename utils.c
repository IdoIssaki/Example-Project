/*
 * קובץ: utils.h
 */
#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include "globals.h"

void *safe_malloc(size_t size);
void skip_whitespaces(char **str);
boolean is_empty_or_comment(char *line);
char *create_file_name(const char *original_name, const char *extension);

#endif
