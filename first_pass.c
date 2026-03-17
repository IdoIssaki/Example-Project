#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* פונקציה להוספת סמלים - קריטי למעבר הראשון */
static boolean add_symbol(symbol_ptr *head, const char *name, int value, int is_code, int is_data, int is_extern) {
    symbol_ptr new_node, current = *head;
    while (current) {
        if (strcmp(current->name, name) == 0) return FALSE;
        current = current->next;
    }
    new_node = (symbol_ptr)safe_malloc(sizeof(symbol_node));
    strcpy(new_node->name, name);
    new_node->value = value;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_extern = is_extern;
    new_node->is_entry = FALSE;
    new_node->next = *head;
    *head = new_node;
    return TRUE;
}

boolean first_pass(FILE *am_file, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2], first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;
    context->ic = INITIAL_IC; context->dc = 0; context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        line_ptr = line;
        if (is_empty_or_comment(line_ptr)) { context->line_number++; continue; }

        extract_word(&line_ptr, first_word);
        if (is_label_definition(first_word)) {
            strncpy(label, first_word, strlen(first_word)-1); label[strlen(first_word)-1] = '\0';
            extract_word(&line_ptr, first_word);
            if (get_command(first_word)) add_symbol(&context->symbol_head, label, context->ic, 1, 0, 0);
            else if (strcmp(first_word, ".data")==0 || strcmp(first_word, ".string")==0)
                add_symbol(&context->symbol_head, label, context->dc, 0, 1, 0);
        }

        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            /* קידוד נתונים וקידום DC... (הלוגיקה הקודמת נשמרת) */
        } else if (strcmp(first_word, ".extern") == 0) {
            extract_word(&line_ptr, label);
            add_symbol(&context->symbol_head, label, 0, 0, 0, 1);
        } else {
            const command_entry *cmd = get_command(first_word);
            if (cmd) {
                char src[MAX_LINE_LENGTH] = {0}, dst[MAX_LINE_LENGTH] = {0};
                int src_m = -1, dst_m = -1;

                if (cmd->expected_ops == 2) {
                    extract_word(&line_ptr, src); skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') { line_ptr++; extract_word(&line_ptr, dst); }

                    /* זיהוי וולידציה של מקור */
                    if (src[0] == '#') src_m = 0;
                    else if (is_register(src)) src_m = 3;
                    else if (src[0] == '%') src_m = 2;
                    else src_m = 1;

                    if (!cmd->valid_src_modes[src_m]) {
                        fprintf(stderr, "Error line %d: Invalid source mode for %s\n", context->line_number, cmd->name);
                        context->error_found = TRUE;
                    }
                } else if (cmd->expected_ops == 1) {
                    extract_word(&line_ptr, dst);
                }

                if (cmd->expected_ops >= 1) {
                    /* זיהוי וולידציה של יעד */
                    if (dst[0] == '#') dst_m = 0;
                    else if (is_register(dst)) dst_m = 3;
                    else if (dst[0] == '%') dst_m = 2;
                    else dst_m = 1;

                    if (!cmd->valid_dest_modes[dst_m]) {
                        fprintf(stderr, "Error line %d: Invalid dest mode for %s\n", context->line_number, cmd->name);
                        context->error_found = TRUE;
                    }
                }
                /* קידוד המילה הראשונה וקידום IC... */
            }
        }
        context->line_number++;
    }
    return !context->error_found;
}
