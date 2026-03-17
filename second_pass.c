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
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH];
    char *line_ptr;

    context->ic = INITIAL_IC;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        /* הצהרת כל המשתנים בתחילת הבלוק לפי תקן C90 */
        char label[MAX_LABEL_LENGTH];
        symbol_ptr sym;
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH];
        char dst[MAX_LINE_LENGTH];
        int src_m, dst_m, L, offset;
        char *target;

        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        /* דילוג על תווית אם קיימת */
        if (is_label_definition(first_word)) {
            extract_word(&line_ptr, first_word);
        }

        /* במעבר שני מתעלמים מהנחיות נתונים ו-extern */
        if (strcmp(first_word, ".data") == 0 ||
            strcmp(first_word, ".string") == 0 ||
            strcmp(first_word, ".extern") == 0) {
            context->line_number++;
            continue;
        }

        if (strcmp(first_word, ".entry") == 0) {
            extract_word(&line_ptr, label);
            sym = get_symbol(context->symbol_head, label);
            if (sym) {
                sym->is_entry = TRUE;
            } else {
                fprintf(stderr, "Error line %d: Entry label '%s' not defined.\n", context->line_number, label);
                context->error_found = TRUE;
            }
        } else {
            cmd = get_command(first_word);
            if (cmd) {
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));
                src_m = -1;
                dst_m = -1;
                L = 1;

                /* חילוץ האופרנדים כדי לחשב את אורך ההוראה L במדויק */
                if (cmd->expected_ops == 2) {
                    extract_word(&line_ptr, src);
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') {
                        line_ptr++;
                        extract_word(&line_ptr, dst);
                    }
                    if (src[0] == '#') src_m = 0;
                    else if (is_register(src)) src_m = 3;
                    else if (src[0] == '%') src_m = 2;
                    else src_m = 1;
                    L++;
                } else if (cmd->expected_ops == 1) {
                    extract_word(&line_ptr, dst);
                }

                if (cmd->expected_ops >= 1) {
                    if (dst[0] == '#') dst_m = 0;
                    else if (is_register(dst)) dst_m = 3;
                    else if (dst[0] == '%') dst_m = 2;
                    else dst_m = 1;
                    L++;
                }

                /* שני רגיסטרים חולקים מילת הרחבה אחת - תואם למעבר הראשון */
                if (src_m == 3 && dst_m == 3) {
                    L--;
                }

                /* השלמת הקידוד עבור אופרנד מקור (אם הוא תווית) */
                if (src_m == 1 || src_m == 2) {
                    target = (src_m == 2) ? src + 1 : src;
                    sym = get_symbol(context->symbol_head, target);
                    if (sym) {
                        if (sym->is_extern) {
                            context->code_image[context->ic - INITIAL_IC + 1].are = ARE_EXTERNAL;
                            add_ext_node(ext_list_head, sym->name, context->ic + 1);
                        } else {
                            context->code_image[context->ic - INITIAL_IC + 1].value = (src_m == 2) ? (sym->value - context->ic) : sym->value;
                            context->code_image[context->ic - INITIAL_IC + 1].are = (src_m == 2) ? ARE_ABSOLUTE : ARE_RELOCATABLE;
                        }
                    } else {
                        fprintf(stderr, "Error line %d: Undefined symbol '%s'\n", context->line_number, target);
                        context->error_found = TRUE;
                    }
                }

                /* השלמת הקידוד עבור אופרנד יעד (אם הוא תווית) */
                if (dst_m == 1 || dst_m == 2) {
                    offset = (src_m != -1) ? 2 : 1;
                    target = (dst_m == 2) ? dst + 1 : dst;
                    sym = get_symbol(context->symbol_head, target);
                    if (sym) {
                        if (sym->is_extern) {
                            context->code_image[context->ic - INITIAL_IC + offset].are = ARE_EXTERNAL;
                            add_ext_node(ext_list_head, sym->name, context->ic + offset);
                        } else {
                            context->code_image[context->ic - INITIAL_IC + offset].value = (dst_m == 2) ? (sym->value - context->ic) : sym->value;
                            context->code_image[context->ic - INITIAL_IC + offset].are = (dst_m == 2) ? ARE_ABSOLUTE : ARE_RELOCATABLE;
                        }
                    } else {
                        fprintf(stderr, "Error line %d: Undefined symbol '%s'\n", context->line_number, target);
                        context->error_found = TRUE;
                    }
                }

                context->ic += L;
            }
        }
        context->line_number++;
    }
    return !context->error_found;
}
