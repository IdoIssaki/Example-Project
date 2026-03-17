#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* פונקציה להוספת סמל לטבלת הסמלים */
static boolean add_symbol(symbol_ptr *head, const char *name, int value, int is_code, int is_data, int is_extern) {
    symbol_ptr new_node;
    symbol_ptr current = *head;

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
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;
    symbol_ptr curr; /* הצהרה בתחילת הבלוק לפי תקן C90 */

    context->ic = INITIAL_IC;
    context->dc = 0;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        boolean symbol_flag = FALSE;
        /* ריכוז כל הצהרות המשתנים לתחילת הבלוק */
        char num_str[MAX_LINE_LENGTH];
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH];
        char dst[MAX_LINE_LENGTH];
        int src_m, dst_m, L;
        int val, offset;

        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        if (is_label_definition(first_word)) {
            symbol_flag = TRUE;
            strncpy(label, first_word, strlen(first_word) - 1);
            label[strlen(first_word) - 1] = '\0';
            extract_word(&line_ptr, first_word);
        }

        /* טיפול בנתונים */
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            if (symbol_flag) {
                if (!add_symbol(&context->symbol_head, label, context->dc, 0, 1, 0)) {
                    fprintf(stderr, "Error line %d: Label '%s' redefined.\n", context->line_number, label);
                    context->error_found = TRUE;
                }
            }
            if (strcmp(first_word, ".data") == 0) {
                extract_word(&line_ptr, num_str);
                while (num_str[0] != '\0') {
                    context->data_image[context->dc].value = atoi(num_str) & 0xFFF;
                    context->data_image[context->dc].are = ARE_ABSOLUTE;
                    context->dc++;
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') {
                        line_ptr++;
                        extract_word(&line_ptr, num_str);
                    } else num_str[0] = '\0';
                }
            } else { /* .string */
                skip_whitespaces(&line_ptr);
                if (*line_ptr == '\"') {
                    line_ptr++;
                    while (*line_ptr != '\"' && *line_ptr != '\0') {
                        context->data_image[context->dc].value = (unsigned int)*line_ptr & 0xFFF;
                        context->data_image[context->dc].are = ARE_ABSOLUTE;
                        context->dc++;
                        line_ptr++;
                    }
                    context->data_image[context->dc].value = 0;
                    context->data_image[context->dc].are = ARE_ABSOLUTE;
                    context->dc++;
                }
            }
        }
        else if (strcmp(first_word, ".extern") == 0) {
            extract_word(&line_ptr, label);
            if (label[0] != '\0') add_symbol(&context->symbol_head, label, 0, 0, 0, 1);
        }
        else if (strcmp(first_word, ".entry") == 0) {
            /* דלג במעבר ראשון */
        }
        else {
            cmd = get_command(first_word);
            if (!cmd) {
                fprintf(stderr, "Error line %d: Unknown command '%s'.\n", context->line_number, first_word);
                context->error_found = TRUE;
            } else {
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));
                src_m = -1;
                dst_m = -1;
                L = 1;

                if (symbol_flag) add_symbol(&context->symbol_head, label, context->ic, 1, 0, 0);

                if (cmd->expected_ops == 2) {
                    extract_word(&line_ptr, src);
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') { line_ptr++; extract_word(&line_ptr, dst); }

                    if (src[0] == '#') src_m = 0;
                    else if (is_register(src)) src_m = 3;
                    else if (src[0] == '%') src_m = 2;
                    else src_m = 1;

                    if (!cmd->valid_src_modes[src_m]) {
                        fprintf(stderr, "Error line %d: Invalid source mode for %s\n", context->line_number, cmd->name);
                        context->error_found = TRUE;
                    }
                    L++;
                } else if (cmd->expected_ops == 1) {
                    extract_word(&line_ptr, dst);
                }

                if (cmd->expected_ops >= 1) {
                    if (dst[0] == '#') dst_m = 0;
                    else if (is_register(dst)) dst_m = 3;
                    else if (dst[0] == '%') dst_m = 2;
                    else dst_m = 1;

                    if (!cmd->valid_dest_modes[dst_m]) {
                        fprintf(stderr, "Error line %d: Invalid dest mode for %s\n", context->line_number, cmd->name);
                        context->error_found = TRUE;
                    }
                    L++;
                }

                /* טיפול מיוחד: שני רגיסטרים חולקים מילת הרחבה אחת */
                if (src_m == 3 && dst_m == 3) {
                    L--;
                }

                context->code_image[context->ic - INITIAL_IC].value =
                        (cmd->opcode << 8) | (cmd->funct << 4) |
                        ((src_m != -1 ? src_m : 0) << 2) |
                        (dst_m != -1 ? dst_m : 0);
                context->code_image[context->ic - INITIAL_IC].are = ARE_ABSOLUTE;

                /* קידוד מילות ההרחבה לפי שיטות המיעון */
                if (src_m == 3 && dst_m == 3) {
                    /* שילוב שני הרגיסטרים למילה אחת (OR ביטווייז) */
                    val = (1 << (src[1] - '0')) | (1 << (dst[1] - '0'));
                    context->code_image[context->ic - INITIAL_IC + 1].value = val & 0xFFF;
                    context->code_image[context->ic - INITIAL_IC + 1].are = ARE_ABSOLUTE;
                } else {
                    if (src_m == 0 || src_m == 3) {
                        val = (src_m == 0) ? atoi(src + 1) : (1 << (src[1] - '0'));
                        context->code_image[context->ic - INITIAL_IC + 1].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + 1].are = ARE_ABSOLUTE;
                    }
                    if (dst_m == 0 || dst_m == 3) {
                        val = (dst_m == 0) ? atoi(dst + 1) : (1 << (dst[1] - '0'));
                        offset = (src_m != -1) ? 2 : 1; /* המיקום תלוי בשאלה אם היה אופרנד מקור */
                        context->code_image[context->ic - INITIAL_IC + offset].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + offset].are = ARE_ABSOLUTE;
                    }
                }
                context->ic += L;
            }
        }
        context->line_number++;
    }

    /* עדכון כתובות הנתונים בסוף המעבר */
    curr = context->symbol_head;
    while (curr) {
        if (curr->is_data) curr->value += context->ic;
        curr = curr->next;
    }

    return !context->error_found;
}
