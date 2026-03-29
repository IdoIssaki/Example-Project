/*
 * קובץ: pre_assembler.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pre_assembler.h"
#include "macro_table.h"
#include "parser.h"
#include "utils.h"

#define MAX_LABELS 500 /* פנקס הזיכרון שלנו לתוויות */

boolean pre_assemble(FILE *source_file, const char *base_file_name, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH + 2];
    char *line_ptr;
    char *am_file_name;
    FILE *am_file;
    char *temp_ptr;
    char second_word[MAX_LINE_LENGTH];
    char macro_name[MAX_LABEL_LENGTH + 2];
    char temp_label[MAX_LABEL_LENGTH];
    macro_line_node *curr_line;
    boolean is_label_conflict;

    boolean is_inside_macro = FALSE;
    macro_ptr macro_head = NULL;
    macro_ptr current_macro = NULL;
    macro_ptr found_macro = NULL;
    int c;

    /* מערך שומר שמות תוויות שכבר ראינו בקובץ */
    char seen_labels[MAX_LABELS][MAX_LABEL_LENGTH];
    int seen_labels_count = 0;
    int i;

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
            fprintf(stderr, "Error at line %d: Line exceeds max length.\n", context->line_number);
            context->error_found = TRUE;
            while ((c = fgetc(source_file)) != '\n' && c != EOF);
            context->line_number++;
            continue;
        }

        line_ptr = line;

        if (is_empty_or_comment(line_ptr)) {
            if (is_inside_macro) {
                if (current_macro != NULL) add_macro_line(current_macro, line);
            }
            /* לא מדפיסים כלום! פשוט מדלגים לשורה הבאה */
            context->line_number++;
            continue;
        }

        extract_word(&line_ptr, first_word);

        /* --- טיפול בתוך בלוק של מאקרו --- */
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
                /* נכנס לכאן גם אם המאקרו תקין וגם אם הוא בסטטוס "התאוששות משגיאה" */
                if (current_macro != NULL) {
                    add_macro_line(current_macro, line);
                }
            }
        }
            /* --- טיפול מחוץ למאקרו (קוד רגיל) --- */
        else {
            temp_ptr = line_ptr;
            memset(second_word, 0, sizeof(second_word));
            extract_word(&temp_ptr, second_word);

            /* זיהוי זבל לפני הצהרת מאקרו */
            if (strcmp(second_word, "mcro") == 0) {
                fprintf(stderr, "Error at line %d: Extraneous text or label ('%s') before 'mcro'.\n", context->line_number, first_word);
                context->error_found = TRUE;
                is_inside_macro = TRUE; /* טריק התאוששות: נכנסים כדי לבלוע את ההמשך */
                current_macro = NULL;
                context->line_number++;
                continue;
            }

            if (strcmp(first_word, "mcro") == 0) {
                memset(macro_name, 0, sizeof(macro_name));
                is_label_conflict = FALSE;
                extract_word(&line_ptr, macro_name);

                /* בדיקה בפנקס התוויות שלנו */
                for (i = 0; i < seen_labels_count; i++) {
                    if (strcmp(seen_labels[i], macro_name) == 0) {
                        is_label_conflict = TRUE;
                        break;
                    }
                }

                if (macro_name[0] == '\0') {
                    fprintf(stderr, "Error at line %d: Missing macro name.\n", context->line_number);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE; current_macro = NULL; /* התאוששות */
                } else if (is_reserved_word(macro_name)) {
                    fprintf(stderr, "Error at line %d: Macro name '%s' is reserved.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE; current_macro = NULL; /* התאוששות */
                } else if (get_macro(macro_head, macro_name) != NULL) {
                    fprintf(stderr, "Error at line %d: Macro '%s' already defined.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE; current_macro = NULL; /* התאוששות */
                } else if (is_label_conflict) {
                    fprintf(stderr, "Error at line %d: Macro name '%s' cannot be identical to an already defined label.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE; current_macro = NULL; /* התאוששות */
                } else {
                    /* המאקרו חוקי! נבדוק רק שאין זבל בסוף השורה */
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr != '\0' && *line_ptr != '\n') {
                        fprintf(stderr, "Error at line %d: Extraneous text after macro name.\n", context->line_number);
                        context->error_found = TRUE;
                    }
                    current_macro = add_macro(&macro_head, macro_name);
                    is_inside_macro = TRUE;
                }
            }
                /* mcroend ללא פתיחה */
            else if (strcmp(first_word, "mcroend") == 0) {
                fprintf(stderr, "Error at line %d: 'mcroend' encountered without matching 'mcro'.\n", context->line_number);
                context->error_found = TRUE;
            }
                /* קוד רגיל או תווית */
            else {
                /* אם זו תווית, נשמור אותה בפנקס שלנו */
                if (strlen(first_word) > 0 && first_word[strlen(first_word) - 1] == ':') {
                    strncpy(temp_label, first_word, strlen(first_word) - 1);
                    temp_label[strlen(first_word) - 1] = '\0';

                    if (get_macro(macro_head, temp_label) != NULL) {
                        fprintf(stderr, "Error at line %d: Label name '%s' cannot be identical to a defined macro name.\n", context->line_number, temp_label);
                        context->error_found = TRUE;
                    } else {
                        if (seen_labels_count < MAX_LABELS) {
                            strcpy(seen_labels[seen_labels_count++], temp_label);
                        }
                    }
                }

                /* פריסת המאקרו אם נקרא (ורק אם הוא חוקי ונשמר) */
                if ((found_macro = get_macro(macro_head, first_word)) != NULL) {
                    curr_line = found_macro->lines_head;
                    while (curr_line != NULL) {
                        fputs(curr_line->line, am_file);
                        curr_line = curr_line->next;
                    }
                } else {
                    fputs(line, am_file);
                }
            }
        }
        context->line_number++;
    }

    fclose(am_file);
    free(am_file_name);
    free_macro_table(macro_head);

    return !context->error_found;
}
