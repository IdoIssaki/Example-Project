#ifndef OUTPUT_GENERATOR_H
#define OUTPUT_GENERATOR_H

#include "globals.h"
#include "second_pass.h"

/* הפונקציה המרכזית שמייצרת את שלושת קובצי הפלט */
void generate_output_files(const char *base_file_name, AssemblerContext *context, ext_ptr ext_list_head);

#endif
