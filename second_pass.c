#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "second_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* חיפוש סמל בטבלה */
static symbol_ptr get_symbol(symbol_ptr head, const char *name) {
    symbol_ptr current = head;
    while (current) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}

/* הוספת שימוש חיצוני לרשימה */
static void add_ext_node(ext_ptr *head, const char *name, int address) {
    ext_ptr new_node = (ext_ptr)safe_malloc(sizeof(ext_node));
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->next = *head;
    *head = new_node;
}

boolean second_pass(FILE *am_file, AssemblerContext *context, ext_ptr *ext_list_head) {
    char line[MAX_LINE_LENGTH + 2], first_word[MAX_LINE_LENGTH];
    char *line_ptr;
    context->ic = INITIAL_IC;

    while (fgets(line, sizeof(line), am_file)) {
        line_ptr = line;
        if (is_empty_or_comment(line_ptr)) continue;

        extract_word(&line_ptr, first_word);
        if (is_label_definition(first_word)) extract_word(&line_ptr, first_word);

        if (strcmp(first_word, ".entry") == 0) {
            char label[MAX_LABEL_LENGTH];
            symbol_ptr sym;
            extract_word(&line_ptr, label);
            sym = get_symbol(context->symbol_head, label);
            if (sym) sym->is_entry = TRUE;
            else {
                fprintf(stderr, "Error: Entry label %s not defined\n", label);
                context->error_found = TRUE;
            }
        } else {
            const command_entry *cmd = get_command(first_word);
            if (cmd) {
                char src[MAX_LINE_LENGTH] = {0}, dst[MAX_LINE_LENGTH] = {0};
                context->ic++; /* דילוג על מילת הפקודה */

                if (cmd->expected_ops == 2) {
                    extract_word(&line_ptr, src);
                    line_ptr++; /* דילוג על פסיק */
                    extract_word(&line_ptr, dst);
                } else if (cmd->expected_ops == 1) {
                    extract_word(&line_ptr, dst);
                }

                /* השלמת מילת מקור */
                if (cmd->expected_ops == 2) {
                    if (src[0] != '#' && !is_register(src)) {
                        char *target = (src[0] == '%') ? src + 1 : src;
                        symbol_ptr sym = get_symbol(context->symbol_head, target);
                        if (sym) {
                            if (sym->is_extern) {
                                context->code_image[context->ic - INITIAL_IC].are = ARE_EXTERNAL;
                                add_ext_node(ext_list_head, sym->name, context->ic);
                            } else {
                                context->code_image[context->ic - INITIAL_IC].value = (src[0] == '%') ? (sym->value - context->ic) : sym->value;
                                context->code_image[context->ic - INITIAL_IC].are = (src[0] == '%') ? ARE_ABSOLUTE : ARE_RELOCATABLE;
                            }
                        }
                    }
                    context->ic++;
                }

                /* השלמת מילת יעד */
                if (cmd->expected_ops >= 1) {
                    if (dst[0] != '#' && !is_register(dst)) {
                        char *target = (dst[0] == '%') ? dst + 1 : dst;
                        symbol_ptr sym = get_symbol(context->symbol_head, target);
                        if (sym) {
                            if (sym->is_extern) {
                                context->code_image[context->ic - INITIAL_IC].are = ARE_EXTERNAL;
                                add_ext_node(ext_list_head, sym->name, context->ic);
                            } else {
                                context->code_image[context->ic - INITIAL_IC].value = (dst[0] == '%') ? (sym->value - context->ic) : sym->value;
                                context->code_image[context->ic - INITIAL_IC].are = (dst[0] == '%') ? ARE_ABSOLUTE : ARE_RELOCATABLE;
                            }
                        }
                    }
                    context->ic++;
                }
            }
        }
    }
    return !context->error_found;
}
