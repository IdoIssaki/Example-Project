/*
 * קובץ: second_pass.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "second_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* פונקציית עזר לחיפוש סמל בטבלה */
static symbol_ptr get_symbol(symbol_ptr head, const char *name) {
    symbol_ptr current = head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }
    return NULL;
}

/* פונקציית עזר להוספת שימוש חיצוני לרשימה */
static void add_ext_node(ext_ptr *head, const char *name, int address) {
    ext_ptr new_node = (ext_ptr)safe_malloc(sizeof(ext_node));
    strncpy(new_node->name, name, MAX_LABEL_LENGTH);
    new_node->name[MAX_LABEL_LENGTH] = '\0';
    new_node->address = address;

    /* הוספה לסוף הרשימה כדי לשמור על סדר ההופעה */
    new_node->next = NULL;
    if (*head == NULL) {
        *head = new_node;
    } else {
        ext_ptr current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

boolean second_pass(FILE *am_file, AssemblerContext *context, ext_ptr *ext_list_head) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH + 2];
    char *line_ptr;

    /* אתחול מחדש של המונים */
    context->ic = INITIAL_IC;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file) != NULL) {
        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        /* דילוג על תווית אם קיימת בתחילת השורה */
        if (is_label_definition(first_word)) {
            extract_word(&line_ptr, first_word);
        }

        /* דילוג על הנחיות שטיפלנו בהן במעבר הראשון */
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0 || strcmp(first_word, ".extern") == 0) {
            context->line_number++;
            continue;
        }

        /* טיפול ב-.entry */
        if (strcmp(first_word, ".entry") == 0) {
            char entry_label[MAX_LABEL_LENGTH + 2];
            symbol_ptr sym;

            extract_word(&line_ptr, entry_label);
            sym = get_symbol(context->symbol_head, entry_label);

            if (sym == NULL) {
                fprintf(stderr, "Error at line %d: .entry directive refers to undefined label '%s'.\n", context->line_number, entry_label);
                context->error_found = TRUE;
            } else {
                sym->is_entry = 1;
            }
        }
            /* טיפול בהוראות - השלמת הקידוד */
        else {
            const command_entry *cmd = get_command(first_word);
            if (cmd != NULL) {
                int expected_ops = 0;
                char src_op[MAX_LINE_LENGTH] = {0}, dest_op[MAX_LINE_LENGTH] = {0};
                int src_mode = 0, dest_mode = 0;
                int has_src = 0, has_dest = 0;

                /* קידום ה-IC עבור מילת הפקודה שכבר קודדה במעבר הראשון */
                context->ic++;

                if (cmd->opcode <= 4 && cmd->opcode != 3) expected_ops = 2;
                else if ((cmd->opcode >= 5 && cmd->opcode <= 13) && cmd->opcode != 6 && cmd->opcode != 7 && cmd->opcode != 8 && cmd->opcode != 10 && cmd->opcode != 11) expected_ops = 1;

                if (expected_ops == 2) {
                    extract_word(&line_ptr, src_op);
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') {
                        line_ptr++;
                        extract_word(&line_ptr, dest_op);
                    }
                } else if (expected_ops == 1) {
                    extract_word(&line_ptr, dest_op);
                }

                if (expected_ops == 2) {
                    has_src = 1;
                    if (src_op[0] == '#') src_mode = 0;
                    else if (is_register(src_op)) src_mode = 3;
                    else if (src_op[0] == '%') src_mode = 2;
                    else src_mode = 1;
                }

                if (expected_ops >= 1) {
                    has_dest = 1;
                    if (dest_op[0] == '#') dest_mode = 0;
                    else if (is_register(dest_op)) dest_mode = 3;
                    else if (dest_op[0] == '%') dest_mode = 2;
                    else dest_mode = 1;
                }

                /* השלמת מילת המקור */
                if (has_src) {
                    if (src_mode == 1 || src_mode == 2) {
                        char *label_to_find = (src_mode == 2) ? src_op + 1 : src_op;
                        symbol_ptr sym = get_symbol(context->symbol_head, label_to_find);

                        if (sym == NULL) {
                            fprintf(stderr, "Error at line %d: Undefined label '%s'.\n", context->line_number, label_to_find);
                            context->error_found = TRUE;
                        } else {
                            if (sym->is_extern) {
                                if (src_mode == 2) {
                                    fprintf(stderr, "Error at line %d: Relative addressing cannot use external label '%s'.\n", context->line_number, label_to_find);
                                    context->error_found = TRUE;
                                } else {
                                    context->code_image[context->ic - INITIAL_IC].value = 0;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_EXTERNAL;
                                    add_ext_node(ext_list_head, sym->name, context->ic);
                                }
                            } else {
                                if (src_mode == 2) {
                                    /* מרחק יחסי: יעד פחות כתובת מילת האופרנד (כתובת ה-IC הנוכחית) */
                                    int distance = sym->value - context->ic;
                                    context->code_image[context->ic - INITIAL_IC].value = distance & 0xFFF;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_ABSOLUTE;
                                } else {
                                    context->code_image[context->ic - INITIAL_IC].value = sym->value & 0xFFF;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_RELOCATABLE;
                                }
                            }
                        }
                    }
                    context->ic++;
                }

                /* השלמת מילת היעד */
                if (has_dest) {
                    if (dest_mode == 1 || dest_mode == 2) {
                        char *label_to_find = (dest_mode == 2) ? dest_op + 1 : dest_op;
                        symbol_ptr sym = get_symbol(context->symbol_head, label_to_find);

                        if (sym == NULL) {
                            fprintf(stderr, "Error at line %d: Undefined label '%s'.\n", context->line_number, label_to_find);
                            context->error_found = TRUE;
                        } else {
                            if (sym->is_extern) {
                                if (dest_mode == 2) {
                                    fprintf(stderr, "Error at line %d: Relative addressing cannot use external label '%s'.\n", context->line_number, label_to_find);
                                    context->error_found = TRUE;
                                } else {
                                    context->code_image[context->ic - INITIAL_IC].value = 0;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_EXTERNAL;
                                    add_ext_node(ext_list_head, sym->name, context->ic);
                                }
                            } else {
                                if (dest_mode == 2) {
                                    int distance = sym->value - context->ic;
                                    context->code_image[context->ic - INITIAL_IC].value = distance & 0xFFF;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_ABSOLUTE;
                                } else {
                                    context->code_image[context->ic - INITIAL_IC].value = sym->value & 0xFFF;
                                    context->code_image[context->ic - INITIAL_IC].are = ARE_RELOCATABLE;
                                }
                            }
                        }
                    }
                    context->ic++;
                }
            }
        }
        context->line_number++;
    }

    return !context->error_found;
}
