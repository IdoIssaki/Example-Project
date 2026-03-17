/*
 * קובץ: first_pass.c
 * מטרת הקובץ: ביצוע המעבר הראשון על קובץ המקור. בניית טבלת הסמלים
 * וקידוד ראשוני של תמונות ההוראות והנתונים.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* פונקציית עזר להוספת סמל לטבלת הסמלים (רשימה מקושרת) */
static boolean add_symbol(symbol_ptr *head, const char *name, int value, int is_code, int is_data, int is_entry, int is_extern) {
    symbol_ptr new_node, current;

    /* בדיקה למניעת כפילויות */
    current = *head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return FALSE;
        }
        current = current->next;
    }

    new_node = (symbol_ptr)safe_malloc(sizeof(symbol_node));
    strncpy(new_node->name, name, MAX_LABEL_LENGTH);
    new_node->name[MAX_LABEL_LENGTH] = '\0';
    new_node->value = value;
    new_node->is_code = is_code;
    new_node->is_data = is_data;
    new_node->is_entry = is_entry;
    new_node->is_extern = is_extern;

    /* הוספה לראש הרשימה ב-O(1) */
    new_node->next = *head;
    *head = new_node;

    return TRUE;
}

boolean first_pass(FILE *am_file, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH + 2];
    char label_name[MAX_LABEL_LENGTH + 2];
    char *line_ptr;
    boolean symbol_flag;

    /* אתחול המונים */
    context->ic = INITIAL_IC;
    context->dc = 0;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file) != NULL) {
        line_ptr = line;
        symbol_flag = FALSE;
        label_name[0] = '\0';

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        /* זיהוי הגדרת תווית */
        if (is_label_definition(first_word)) {
            symbol_flag = TRUE;
            strncpy(label_name, first_word, strlen(first_word) - 1);
            label_name[strlen(first_word) - 1] = '\0';
            extract_word(&line_ptr, first_word); /* קריאת המילה הבאה */
        }

        /* 1. טיפול בהנחיות אחסון נתונים (.data או .string) */
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            if (symbol_flag) {
                if (!add_symbol(&(context->symbol_head), label_name, context->dc, 0, 1, 0, 0)) {
                    fprintf(stderr, "Error at line %d: Label '%s' is already defined.\n", context->line_number, label_name);
                    context->error_found = TRUE;
                }
            }

            if (strcmp(first_word, ".data") == 0) {
                char num_str[MAX_LINE_LENGTH];
                while (*line_ptr != '\0' && *line_ptr != '\n') {
                    extract_word(&line_ptr, num_str);
                    if (strlen(num_str) > 0) {
                        context->data_image[context->dc].value = atoi(num_str) & 0xFFF;
                        context->data_image[context->dc].are = ARE_ABSOLUTE;
                        context->dc++;
                    }
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') {
                        line_ptr++;
                    } else if (*line_ptr != '\0' && *line_ptr != '\n') {
                        fprintf(stderr, "Error at line %d: Invalid syntax in .data directive.\n", context->line_number);
                        context->error_found = TRUE;
                        break;
                    }
                }
            }
            else if (strcmp(first_word, ".string") == 0) {
                skip_whitespaces(&line_ptr);
                if (*line_ptr == '\"') {
                    line_ptr++;
                    while (*line_ptr != '\0' && *line_ptr != '\"' && *line_ptr != '\n') {
                        context->data_image[context->dc].value = (*line_ptr) & 0xFFF;
                        context->data_image[context->dc].are = ARE_ABSOLUTE;
                        context->dc++;
                        line_ptr++;
                    }
                    if (*line_ptr == '\"') {
                        line_ptr++;
                        context->data_image[context->dc].value = 0; /* סימון סוף מחרוזת */
                        context->data_image[context->dc].are = ARE_ABSOLUTE;
                        context->dc++;
                    } else {
                        fprintf(stderr, "Error at line %d: Missing closing quote in .string.\n", context->line_number);
                        context->error_found = TRUE;
                    }
                } else {
                    fprintf(stderr, "Error at line %d: Missing opening quote in .string.\n", context->line_number);
                    context->error_found = TRUE;
                }
            }
        }

            /* 2. טיפול בהנחיות .extern ו-.entry */
        else if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            if (strcmp(first_word, ".extern") == 0) {
                char extern_label[MAX_LABEL_LENGTH + 2];
                extract_word(&line_ptr, extern_label);

                if (!add_symbol(&(context->symbol_head), extern_label, 0, 0, 0, 0, 1)) {
                    fprintf(stderr, "Error at line %d: Label '%s' is already defined.\n", context->line_number, extern_label);
                    context->error_found = TRUE;
                }
            }
            /* להנחיית .entry אין משמעות במעבר הראשון, היא תטופל במעבר השני */
        }

            /* 3. טיפול בשורות הוראה (Instructions) */
        else {
            const command_entry *cmd = get_command(first_word);
            if (cmd == NULL) {
                fprintf(stderr, "Error at line %d: Unknown command '%s'.\n", context->line_number, first_word);
                context->error_found = TRUE;
            } else {
                int expected_ops = 0;
                char src_op[MAX_LINE_LENGTH] = {0}, dest_op[MAX_LINE_LENGTH] = {0};
                int src_mode = 0, dest_mode = 0;
                int src_val = 0, dest_val = 0;
                int has_src = 0, has_dest = 0;
                int ic_offset;

                if (symbol_flag) {
                    if (!add_symbol(&(context->symbol_head), label_name, context->ic, 1, 0, 0, 0)) {
                        fprintf(stderr, "Error at line %d: Label '%s' is already defined.\n", context->line_number, label_name);
                        context->error_found = TRUE;
                    }
                }

                /* קביעת כמות אופרנדים לפי קוד פעולה */
                if (cmd->opcode <= 4 && cmd->opcode != 3) expected_ops = 2; /* 0,1,2,4 */
                else if ((cmd->opcode >= 5 && cmd->opcode <= 13) && cmd->opcode != 6 && cmd->opcode != 7 && cmd->opcode != 8 && cmd->opcode != 10 && cmd->opcode != 11) expected_ops = 1; /* 5,9,12,13 */

                /* חילוץ האופרנדים */
                if (expected_ops == 2) {
                    extract_word(&line_ptr, src_op);
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') {
                        line_ptr++;
                        extract_word(&line_ptr, dest_op);
                    } else {
                        fprintf(stderr, "Error at line %d: Missing comma between operands.\n", context->line_number);
                        context->error_found = TRUE;
                    }
                } else if (expected_ops == 1) {
                    extract_word(&line_ptr, dest_op);
                }

                /* פענוח שיטות מיעון לאופרנד מקור (אם קיים) */
                if (expected_ops == 2) {
                    has_src = 1;
                    if (src_op[0] == '#') { src_mode = 0; src_val = atoi(src_op + 1); }
                    else if (is_register(src_op)) { src_mode = 3; src_val = 1 << (src_op[1] - '0'); }
                    else if (src_op[0] == '%') { src_mode = 2; }
                    else { src_mode = 1; }
                }

                /* פענוח שיטות מיעון לאופרנד יעד (אם קיים) */
                if (expected_ops >= 1) {
                    has_dest = 1;
                    if (dest_op[0] == '#') { dest_mode = 0; dest_val = atoi(dest_op + 1); }
                    else if (is_register(dest_op)) { dest_mode = 3; dest_val = 1 << (dest_op[1] - '0'); }
                    else if (dest_op[0] == '%') { dest_mode = 2; }
                    else { dest_mode = 1; }
                }

                /* קידוד המילה הראשונה של ההוראה */
                ic_offset = context->ic - INITIAL_IC;
                context->code_image[ic_offset].value = (cmd->opcode << 8) | (cmd->funct << 4) | (src_mode << 2) | dest_mode;
                context->code_image[ic_offset].are = ARE_ABSOLUTE;
                context->ic++;

                /* קידוד מילות מידע נוספות */
                if (has_src) {
                    ic_offset = context->ic - INITIAL_IC;
                    if (src_mode == 0 || src_mode == 3) {
                        context->code_image[ic_offset].value = src_val & 0xFFF;
                        context->code_image[ic_offset].are = ARE_ABSOLUTE;
                    } else {
                        /* משאירים מאופס למעבר השני (תוויות) */
                        context->code_image[ic_offset].value = 0;
                        context->code_image[ic_offset].are = 0;
                    }
                    context->ic++;
                }

                if (has_dest) {
                    ic_offset = context->ic - INITIAL_IC;
                    if (dest_mode == 0 || dest_mode == 3) {
                        context->code_image[ic_offset].value = dest_val & 0xFFF;
                        context->code_image[ic_offset].are = ARE_ABSOLUTE;
                    } else {
                        /* משאירים מאופס למעבר השני (תוויות) */
                        context->code_image[ic_offset].value = 0;
                        context->code_image[ic_offset].are = 0;
                    }
                    context->ic++;
                }
            }
        }
        context->line_number++;
    }

    /* שלב סופי: עדכון הערך של סמלי נתונים */
    /* תמונת הנתונים ממוקמת בזיכרון אחרי תמונת ההוראות, ולכן יש להוסיף את ה-IC הסופי */
    {
        symbol_ptr current = context->symbol_head;
        while (current != NULL) {
            if (current->is_data) {
                current->value += context->ic;
            }
            current = current->next;
        }
    }

    return !context->error_found;
}