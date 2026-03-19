#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "second_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

static void add_ext(ext_ptr *head, const char *name, int address) {
    ext_ptr new_node = (ext_ptr)safe_malloc(sizeof(ext_node));
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        ext_ptr temp = *head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = new_node;
    }
}

boolean second_pass(FILE *am_file, AssemblerContext *context, ext_ptr *ext_list_head) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;

    context->ic = INITIAL_IC;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH], dst[MAX_LINE_LENGTH];
        int src_m = -1, dst_m = -1, L = 1, offset, k;

        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        if (is_label_definition(first_word)) {
            extract_word(&line_ptr, first_word);
        }

        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0 || strcmp(first_word, ".extern") == 0) {
            context->line_number++;
            continue;
        }
        else if (strcmp(first_word, ".entry") == 0) {
            symbol_ptr sym;
            extract_word(&line_ptr, label);
            sym = get_symbol(context->symbol_head, label);
            if (sym) sym->is_entry = TRUE;
            else {
                fprintf(stderr, "Error line %d: Undefined entry symbol '%s'\n", context->line_number, label);
                context->error_found = TRUE;
            }
        }
        else {
            cmd = get_command(first_word);
            if (cmd) {
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));

                /* אותו פיענוח בדיוק כמו במעבר הראשון */
                if (cmd->expected_ops == 2) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        src[k++] = *line_ptr++;
                    }
                    src[k] = '\0';
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') line_ptr++;
                    skip_whitespaces(&line_ptr);
                    k = 0;
                    while (*line_ptr && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    src_m = (src[0] == '#') ? 0 : ((strlen(src) == 2 && src[0] == 'r' && src[1] >= '0' && src[1] <= '7') ? 3 : (src[0] == '%' ? 2 : 1));
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));
                    L = 3;
                }
                else if (cmd->expected_ops == 1) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));
                    L = 2;
                }

                /* השלמת כתובות במעבר השני */
                if (src_m == 1 || src_m == 2) {
                    char *sym_name = (src_m == 2) ? src + 1 : src;
                    symbol_ptr sym = get_symbol(context->symbol_head, sym_name);

                    if (sym) {
                        if (sym->is_extern) {
                            context->code_image[context->ic - INITIAL_IC + 1].value = 0;
                            context->code_image[context->ic - INITIAL_IC + 1].are = 'E';
                            add_ext(ext_list_head, sym->name, context->ic + 1);
                        } else {
                            if (src_m == 2) {
                                context->code_image[context->ic - INITIAL_IC + 1].value = (sym->value - context->ic) & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + 1].are = 'A';
                            } else {
                                context->code_image[context->ic - INITIAL_IC + 1].value = sym->value & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + 1].are = 'R';
                            }
                        }
                    }
                }

                if (dst_m == 1 || dst_m == 2) {
                    char *sym_name = (dst_m == 2) ? dst + 1 : dst;
                    symbol_ptr sym = get_symbol(context->symbol_head, sym_name);
                    offset = (src_m != -1) ? 2 : 1;

                    if (sym) {
                        if (sym->is_extern) {
                            context->code_image[context->ic - INITIAL_IC + offset].value = 0;
                            context->code_image[context->ic - INITIAL_IC + offset].are = 'E';
                            add_ext(ext_list_head, sym->name, context->ic + offset);
                        } else {
                            if (dst_m == 2) {
                                context->code_image[context->ic - INITIAL_IC + offset].value = (sym->value - context->ic) & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + offset].are = 'A';
                            } else {
                                context->code_image[context->ic - INITIAL_IC + offset].value = sym->value & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + offset].are = 'R';
                            }
                        }
                    }
                }

                context->ic += L;
            }
        }
        context->line_number++;
    }

    return !context->error_found;
}
