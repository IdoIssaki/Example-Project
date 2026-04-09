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

// כנראה נצטרך בסוף לעשות רשימה מקושרת של תוויות במקום להגביל את כמות התוויות.
#define MAX_LABELS 500 /* פנקס הזיכרון שלנו לתוויות */

boolean pre_assemble(FILE *source_file, const char *base_file_name, AssemblerContext *context) {
    //הפלוס 2 זה להורדת שורה /n וגם ל/0.
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
        //גודל השורה המקסימלי שהוגדר הוא 80 תווים ולכן הוא מחפש רק ב80 תווים הראשונים.
        if (strchr(line, '\n') == NULL && !feof(source_file)) {
            fprintf(stderr, "Error at line %d: Line exceeds max length.\n", context->line_number);
            context->error_found = TRUE;
            //אם המשתמש הכניס יותר ממקסימום התווים הנדרשים, צריך לנקות את השורות ולחפש איפה נמצאת הירידת שורה כדי לעבור לשורה הבאה.
            //עד שהקובץ מסתיים כמובן
            while ((c = fgetc(source_file)) != '\n' && c != EOF);
            context->line_number++;
            continue;
        }

        line_ptr = line;

        //הוספת שורות הערה שבתוך מאקרויים (אם המאקרו קיים)
        if (is_empty_or_comment(line_ptr)) {
            //  לפי הההערות לא נדרש כי זו שורת הערה- לוודא כי יש מצב אפשר למחוק את תהנאי ולא להוסיף את השורה למאקרו.
            if (is_inside_macro) {
                if (current_macro != NULL) add_macro_line(current_macro, line);
            }
            /* לא מדפיסים כלום! פשוט מדלגים לשורה הבאה */
            context->line_number++;
            continue;
        }
        // הוצאת המילה הראשונה בשורה שאנחנו עומדים עליה
        extract_word(&line_ptr, first_word);

        /* --- טיפול בתוך בלוק של מאקרו --- */
        if (is_inside_macro) {
            //מאקרו לא תקין- טקסט אחרי macroend
            if (strcmp(first_word, "mcroend") == 0) {
                skip_whitespaces(&line_ptr);
                if (*line_ptr != '\0' && *line_ptr != '\n') {
                    fprintf(stderr, "Error at line %d: Extraneous text after 'mcroend'.\n", context->line_number);
                    context->error_found = TRUE;
                }
                is_inside_macro = FALSE;
                current_macro = NULL;

            //הדפסת השורה במאקרו
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
            /*
            //אפשר לכתוב במקטם memset את הלולאה הזו לאתחול המערך הזה.
             //רק צריך לשים לב שהint i מחוץ לבלוק ובתחילת התוכנית איפשה שמכריזים על משתנים.
             * int i;
               for (i = 0; i < sizeof(second_word); i++) {
                    second_word[i] = '\0';
}
             */
            // הוצאת המילה השנייה- אחרי שתיגמר ישארו '/0'
            extract_word(&temp_ptr, second_word);

            /* זיהוי זבל לפני הצהרת מאקרו- אם במילה השנייה רק יש את המילה מאקרו במילה הראשונה היה "זבל" */
            if (strcmp(second_word, "mcro") == 0) {
                fprintf(stderr, "Error at line %d: Extraneous text or label ('%s') before 'mcro'.\n", context->line_number, first_word);
                context->error_found = TRUE;
                is_inside_macro = TRUE; /* טריק התאוששות: נכנסים כדי לבלוע את ההמשך */
                current_macro = NULL;
                context->line_number++;
                continue;
            }

            //הוצאת המילה הראשונה- הכרזת מאקרו.
            if (strcmp(first_word, "mcro") == 0) {
                memset(macro_name, 0, sizeof(macro_name));
                is_label_conflict = FALSE;
                // הואצת השם של מאקרו לmacro_name. מצביע על המילה השנייה כי בפונקציה המצביע לשורה התקדם בפעם האחרונה שהשתמשנו בו.
                extract_word(&line_ptr, macro_name);

                /* בדיקה בפנקס התוויות שלנו */
                //אם המילה הזו היא שם של תווית.
                for (i = 0; i < seen_labels_count; i++) {
                    if (strcmp(seen_labels[i], macro_name) == 0) {
                        is_label_conflict = TRUE;
                        break;
                    }
                }


                // בדיקת שיגאות לשם המאקרו בקוד.

                //אין שם למאקרו
                if (macro_name[0] == '\0') {
                    fprintf(stderr, "Error at line %d: Missing macro name.\n", context->line_number);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE;
                    current_macro = NULL; /* התאוששות */ }

                //השם של המאקרו הוא מילה שמורה- למשל שם של פקודה.
                else if (is_reserved_word(macro_name)) {
                    fprintf(stderr, "Error at line %d: Macro name '%s' is reserved.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE;
                    current_macro = NULL; /* התאוששות */ }

                //אם כבר בטבלת המאקרוים קיים מאקרו אחר עם השם הזה, לא ניתן להגדיר את המאקרו הנוכחי.
                else if (get_macro(macro_head, macro_name) != NULL) {
                    fprintf(stderr, "Error at line %d: Macro '%s' already defined.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE;
                    current_macro = NULL; /* התאוששות */ }

                //אם שם המאקרו זהה לשם של תווית.
                else if (is_label_conflict) {
                    fprintf(stderr, "Error at line %d: Macro name '%s' cannot be identical to an already defined label.\n", context->line_number, macro_name);
                    context->error_found = TRUE;
                    is_inside_macro = TRUE;
                    current_macro = NULL; /* התאוששות */ }

                //המאקרו חוקי.
                else {
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

            //בדיקה עבור השורה עם סיום המאקרו אבל כשהדגל של is_inside_macro הוא false. לכן זו שגיאה כי סגרנו מאקרו מבלי לפתוח.
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

                    //הוכרז מאקרו עם השם הזה לכן לא יכולה להיו תווית עם אותו השם.
                    if (get_macro(macro_head, temp_label) != NULL) {
                        fprintf(stderr, "Error at line %d: Label name '%s' cannot be identical to a defined macro name.\n", context->line_number, temp_label);
                        context->error_found = TRUE;

                    //העתקת המערך תוויות למערך תוויות המקורי רק אם מספר התוויות לא יותר מהמקסימום
                    // (כנראה מיותר כי נצטרך לעשות רשימה של תוויות ולאה לה גביל כמות).
                    } else {
                        if (seen_labels_count < MAX_LABELS) {
                            strcpy(seen_labels[seen_labels_count++], temp_label);
                        }
                    }
                }

                /* פריסת המאקרו אם נקרא (ורק אם הוא חוקי ונשמר) */
                //הדפסת שורות המאקרויים החוקיים
                if ((found_macro = get_macro(macro_head, first_word)) != NULL) {
                    curr_line = found_macro->lines_head;
                    while (curr_line != NULL) {
                        fputs(curr_line->line, am_file);
                        curr_line = curr_line->next;
                    }
                }
                // שורת קוד רגילה ולא מאקרו ולכן תיכתב בקובץ כרגיל.
                // לבדוק האם הערות עם ; בסוף נכתבות גם- ואם כן לבדוק בהנחיות אם צריך שיכתבו או לא.
                else {
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
