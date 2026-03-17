/*
 * קובץ: main.c (מיועד לבדיקת שלב הקדם-אסמבלר בלבד)
 */
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "utils.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "second_pass.h"

int main(int argc, char *argv[]) {
    int i;
    FILE *source_file;
    char *file_name;
    AssemblerContext current_context;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        file_name = create_file_name(argv[i], ".as");
        source_file = fopen(file_name, "r");
        
        if (source_file == NULL) {
            fprintf(stderr, "Error: Cannot open file '%s'. Skipping to next.\n", file_name);
            free(file_name);
            continue;
        }

        /* אתחול ההקשר ללא משתנים גלובליים */
        current_context.ic = INITIAL_IC; 
        current_context.dc = 0;
        current_context.line_number = 1;
        current_context.error_found = FALSE;
        current_context.symbol_head = NULL;

        printf("Processing macros for file: %s\n", file_name);

        /* הפעלת שלב הפרישה בלבד */
        if (pre_assemble(source_file, argv[i], &current_context)) {
            /* 1. קודם כל הכרזת משתנים בתחילת הבלוק */
            char *am_file_name;
            FILE *am_file;

            /* 2. רק אז שורות קוד וקריאות לפונקציות */
            printf("Success! Generated %s.am\n", argv[i]);

            /* --- הפעלת המעברים --- */
            am_file_name = create_file_name(argv[i], ".am");
            am_file = fopen(am_file_name, "r");

            if (am_file != NULL) {
                printf("Starting first pass for: %s\n", am_file_name);
                if (first_pass(am_file, &current_context)) {
                    printf("First pass completed successfully!\n");

                    /* --- הפעלת המעבר השני --- */
                    /* חובה: החזרת סמן הקובץ להתחלה לפני המעבר השני */
                    rewind(am_file);

                    /* הגדרת ראש הרשימה המקושרת לסמלים חיצוניים */
                    ext_ptr ext_list_head = NULL;

                    printf("Starting second pass...\n");
                    if (second_pass(am_file, &current_context, &ext_list_head)) {
                        printf("Second pass completed successfully!\n");

                        /* כאן יבוא השלב הבא והאחרון: ייצור קובצי הפלט */
                        /* (קובצי .ob, .ent, .ext) */

                    } else {
                        printf("Failed: Errors found during second pass.\n");
                    }

                } else {
                    printf("Failed: Errors found during first pass.\n");
                }
                fclose(am_file);
            } else {
                fprintf(stderr, "Error: Cannot open .am file for passes.\n");
            }

            /* החשוב מכל - שחרור הזיכרון! בדיוק איפה שהוא צריך להיות */
            free(am_file_name);

        } else {
            /* אם הפרישה נכשלה */
            printf("Failed: Errors found in %s\n", file_name);
        }


        fclose(source_file);
        free(file_name);
    }

    return EXIT_SUCCESS;
}

