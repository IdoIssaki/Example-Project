#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

boolean first_pass(FILE *am_file, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;
    symbol_ptr curr;
    boolean expect_number;

    context->ic = INITIAL_IC;
    context->dc = 0;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        boolean symbol_flag = FALSE;
        char num_str[MAX_LINE_LENGTH];
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH];
        char dst[MAX_LINE_LENGTH];
        int src_m = -1, dst_m = -1, L = 1;
        int val, offset, k;

        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        /* --- בדיקה אבסולוטית: האם התווית מתחילה באות? --- */
        if (strlen(first_word) > 0 && first_word[strlen(first_word) - 1] == ':') {
            if (!((first_word[0] >= 'A' && first_word[0] <= 'Z') || (first_word[0] >= 'a' && first_word[0] <= 'z'))) {
                fprintf(stderr, "Error line %d: Invalid label '%s' (Label MUST start with a letter)\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            if (!is_label_definition(first_word)) {
                fprintf(stderr, "Error line %d: Invalid label syntax '%s'\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            symbol_flag = TRUE;
            strncpy(label, first_word, strlen(first_word) - 1);
            label[strlen(first_word) - 1] = '\0';
            extract_word(&line_ptr, first_word);
        }

        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            if (symbol_flag) {
                if (!add_symbol(&context->symbol_head, label, context->dc, 0, 1, 0)) {
                    fprintf(stderr, "Error line %d: Label '%s' redefined.\n", context->line_number, label);
                    context->error_found = TRUE;
                }
            }
            if (strcmp(first_word, ".data") == 0) {
                expect_number = TRUE;
                while (*line_ptr != '\0' && *line_ptr != '\n') {
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == '\0' || *line_ptr == '\n') break;
                    
                    if (*line_ptr == ',') {
                        if (expect_number) {
                            fprintf(stderr, "Error line %d: Unexpected comma in .data\n", context->line_number);
                            context->error_found = TRUE;
                            break;
                        }
                        line_ptr++;
                        expect_number = TRUE;
                        continue;
                    }
                    
                    if (!expect_number) {
                        fprintf(stderr, "Error line %d: Expected comma between numbers\n", context->line_number);
                        context->error_found = TRUE;
                        break;
                    }
                    
                    extract_word(&line_ptr, num_str);
                    if (num_str[0] != '\0') {
                        if (!is_valid_number(num_str)) {
                            fprintf(stderr, "Error line %d: Invalid number '%s' in .data\n", context->line_number, num_str);
                            context->error_found = TRUE;
                            break;
                        }
                        context->data_image[context->dc].value = atoi(num_str) & 0xFFF;
                        context->data_image[context->dc].are = 'A';
                        context->dc++;
                        expect_number = FALSE;
                    }
                }
            } else { /* טיפול ב- .string */
                skip_whitespaces(&line_ptr);

                /* 1. בדיקה שיש מרכאה פותחת */
                if (*line_ptr != '\"') {
                    fprintf(stderr, "Error line %d: Missing opening quote for .string directive\n", context->line_number);
                    context->error_found = TRUE;
                } else {
                    line_ptr++; /* מדלגים על המרכאה הפותחת */

                    /* מכניסים את התווים למערך */
                    while (*line_ptr != '\"' && *line_ptr != '\0' && *line_ptr != '\n') {
                        context->data_image[context->dc].value = (unsigned int)*line_ptr & 0xFFF;
                        context->data_image[context->dc].are = 'A';
                        context->dc++;
                        line_ptr++;
                    }

                    /* 2. הגענו לסוף - בודקים אם עצרנו בגלל מרכאה סוגרת או בגלל שנגמרה השורה */
                    if (*line_ptr != '\"') {
                        fprintf(stderr, "Error line %d: Missing closing quote for .string directive\n", context->line_number);
                        context->error_found = TRUE;
                    } else {
                        /* סיימנו בהצלחה! סוגרים את המחרוזת עם תו ה-NULL (אפס) */
                        context->data_image[context->dc].value = 0;
                        context->data_image[context->dc].are = 'A';
                        context->dc++;

                        line_ptr++; /* מדלגים על המרכאה הסוגרת */

                        /* 3. בדיקה שאין טקסט מיותר אחרי המחרוזת */
                        skip_whitespaces(&line_ptr);
                        if (*line_ptr != '\0' && *line_ptr != '\n') {
                            fprintf(stderr, "Error line %d: Extraneous text after string in .string directive\n", context->line_number);
                            context->error_found = TRUE;
                        }
                    }
                }
            }
        }
        else if (strcmp(first_word, ".extern") == 0) {
            extract_word(&line_ptr, label);
            if (label[0] != '\0') {
                /* בדיקה שהאקסטרן מתחיל באות */
                if (!((label[0] >= 'A' && label[0] <= 'Z') || (label[0] >= 'a' && label[0] <= 'z'))) {
                    fprintf(stderr, "Error line %d: Invalid extern label '%s' (Must start with a letter)\n", context->line_number, label);
                    context->error_found = TRUE;
                } else {
                    /* --- הנה התיקון שלנו: בודקים אם הסמל כבר קיים בבית --- */
                    symbol_ptr existing = get_symbol(context->symbol_head, label);

                    if (existing != NULL) {
                        /* מצאנו אותו בטבלה! האם הוא הוגדר פה כמקומי? */
                        if (!existing->is_extern) {
                            fprintf(stderr, "Error line %d: Symbol '%s' is already defined locally. Cannot declare as .extern\n", context->line_number, label);
                            context->error_found = TRUE;
                        }
                    } else {
                        /* הוא לא קיים עדיין, אז מותר להגדיר אותו כחיצוני בבטחה */
                        add_symbol(&context->symbol_head, label, 0, 0, 0, 1);
                    }
                }
            }
        }
        else if (strcmp(first_word, ".entry") == 0) {
            /* המעבר השני מטפל בזה */
        }
        else {
            cmd = get_command(first_word);
            if (!cmd) {
                fprintf(stderr, "Error line %d: Unknown command '%s'.\n", context->line_number, first_word);
                context->error_found = TRUE;
            } else {
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));

                if (symbol_flag) {
                    /* מנסים להוסיף, ואם הפונקציה מחזירה שקר - סימן שהתווית כבר קיימת! */
                    if (!add_symbol(&context->symbol_head, label, context->ic, 1, 0, 0)) {
                        fprintf(stderr, "Error line %d: Label '%s' is already defined.\n", context->line_number, label);
                        context->error_found = TRUE;
                    }
                }

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
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    if (strlen(src) == 0 || strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand(s) for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    src_m = (src[0] == '#') ? 0 : ((strlen(src) == 2 && src[0] == 'r' && src[1] >= '0' && src[1] <= '7') ? 3 : (src[0] == '%' ? 2 : 1));
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));
                    
                    /* בדיקת שיטות מיעון מותרות */
                    if (!is_valid_addr_mode(cmd->valid_src_modes, src_m)) {
                        fprintf(stderr, "Error line %d: Invalid source addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    if (!is_valid_addr_mode(cmd->valid_dest_modes, dst_m)) {
                        fprintf(stderr, "Error line %d: Invalid dest addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    
                    /* בדיקת ערך מיידי תקין */
                    if (src_m == 0 && !is_valid_number(src + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, src);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    if (dst_m == 0 && !is_valid_number(dst + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, dst);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    
                    L = 3;
                }
                else if (cmd->expected_ops == 1) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    if (strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));
                    
                    /* בדיקת שיטת מיעון מותרת */
                    if (!is_valid_addr_mode(cmd->valid_dest_modes, dst_m)) {
                        fprintf(stderr, "Error line %d: Invalid dest addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    
                    /* בדיקת ערך מיידי תקין */
                    if (dst_m == 0 && !is_valid_number(dst + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, dst);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    
                    L = 2;
                }

                /* --- בדיקת טקסט מיותר לכל הפקודות (כולל stop שמקבלת 0) --- */
                skip_whitespaces(&line_ptr);
                if (*line_ptr != '\0' && *line_ptr != '\n') {
                    fprintf(stderr, "Error line %d: Too many operands or extraneous text for command '%s'\n", context->line_number, first_word);
                    context->error_found = TRUE;
                    context->line_number++;
                    continue;
                }

                context->code_image[context->ic - INITIAL_IC].value =
                        (cmd->opcode << 8) |
                        (cmd->funct << 4) |
                        ((src_m != -1 ? src_m : 0) << 2) |
                        (dst_m != -1 ? dst_m : 0);
                context->code_image[context->ic - INITIAL_IC].are = 'A';

                if (src_m != -1) {
                    if (src_m == 0 || src_m == 3) {
                        val = (src_m == 0) ? atoi(src + 1) : (1 << (src[1] - '0'));
                        context->code_image[context->ic - INITIAL_IC + 1].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + 1].are = 'A';
                    } else {
                        context->code_image[context->ic - INITIAL_IC + 1].value = 0;
                        context->code_image[context->ic - INITIAL_IC + 1].are = '\0';
                    }
                }
                if (dst_m != -1) {
                    offset = (src_m != -1) ? 2 : 1;
                    if (dst_m == 0 || dst_m == 3) {
                        val = (dst_m == 0) ? atoi(dst + 1) : (1 << (dst[1] - '0'));
                        context->code_image[context->ic - INITIAL_IC + offset].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + offset].are = 'A';
                    } else {
                        context->code_image[context->ic - INITIAL_IC + offset].value = 0;
                        context->code_image[context->ic - INITIAL_IC + offset].are = '\0';
                    }
                }

                context->ic += L;
            }
        }
        context->line_number++;
    }

    curr = context->symbol_head;
    while (curr) {
        if (curr->is_data) curr->value += context->ic;
        curr = curr->next;
    }

    return !context->error_found;
}
