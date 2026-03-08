/*
 * קובץ: pre_assembler.c
 * מטרת הקובץ: מימוש האלגוריתם לפרישת מאקרואים.
 * כיצד מושגת המטרה: קריאת קובץ המקור, בניית טבלת מאקרו, והחלפת קריאות במלל.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pre_assembler.h"
#include "macro_table.h"
#include "parser.h"
#include "utils.h"

boolean pre_assemble(FILE *source_file, const char *base_file_name, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH + 2];
    char *line_ptr;
    char *am_file_name;
    FILE *am_file;
    
    boolean is_inside_macro = FALSE;
    macro_ptr macro_head = NULL; 
    macro_ptr current_macro = NULL;
    macro_ptr found_macro = NULL;
    int c;

    context->line_number = 1;
    context->error_found = FALSE;

    am_file_name = create_file_name(base_file_name, ".am");
    am_file = fopen(am_file_name, "w");
    if (am_file == NULL) {
        fprintf(stderr, "Error: Cannot create output file %s\n", am_file_name);
        free(am_file_name);
        return FALSE;
    }

    while (fgets(line, sizeof(line), source_file) != NULL) {
        if (strchr(line, '\n') == NULL && !feof(source_file)) {
            fprintf(stderr, "Error at line %d: Line exceeds maximum length of %d characters.\n", 
                    context->line_number, MAX_LINE_LENGTH);
            context->error_found = TRUE;
            while ((c = fgetc(source_file)) != '\n' && c != EOF);
            context->line_number++;
            continue;
        }

        line_ptr = line;
        
        if (is_empty_or_comment(line_ptr)) {
            if (is_inside_macro) {
                add_macro_line(current_macro, line);
            } else {
                fputs(line, am_file);
            }
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        if (is_inside_macro) {
            if (strcmp(first_word, "mcroend") == 0) {
                skip_whitespaces(&line_ptr);
                if (*line_ptr != '\0' && *line_ptr != '\n') {
                    fprintf(stderr, "Error at line %d: Extraneous text after 'mcroend'.\n", context->line_number);
                    context->error_found = TRUE;
                }
                is_inside_macro = FALSE;
                current_macro = NULL;
            } else {
                add_macro_line(current_macro, line);
            }
        } else {
            if (strcmp(first_word, "mcro") == 0) {
                char macro_name[MAX_LABEL_LENGTH + 2] = {0};
                extract_word(&line_ptr, macro_name);
                
                if (macro_name[0] == '\0') {
                    fprintf(stderr, "Error at line %d: Missing macro name.\n", context->line_number);
                    context->error_found = TRUE;
                } else if (is_reserved_word(macro_name)) {
                    fprintf(stderr, "Error at line %d: Macro name '%s' is reserved.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                } else if (get_macro(macro_head, macro_name) != NULL) {
                    fprintf(stderr, "Error at line %d: Macro '%s' already defined.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                } else {
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr != '\0' && *line_ptr != '\n') {
                        fprintf(stderr, "Error at line %d: Extraneous text after macro name.\n", context->line_number);
                        context->error_found = TRUE;
                    }
                    current_macro = add_macro(&macro_head, macro_name);
                    is_inside_macro = TRUE;
                }
            } 
            else if ((found_macro = get_macro(macro_head, first_word)) != NULL) {
                macro_line_node *curr_line = found_macro->lines_head;
                while (curr_line != NULL) {
                    fputs(curr_line->line, am_file);
                    curr_line = curr_line->next;
                }
            } 
            else {
                fputs(line, am_file);
            }
        }
        context->line_number++;
    }

    fclose(am_file);
    free(am_file_name);
    free_macro_table(macro_head); 

    return !context->error_found;
}
