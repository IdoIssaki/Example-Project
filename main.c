/*
 * קובץ: main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "utils.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "second_pass.h"
#include "output_generator.h"

int main(int argc, char *argv[]) {
    /* ריכוז הצהרות משתנים לתחילת הבלוק לפי תקן C90 */
    int i;
    FILE *source_file;
    FILE *am_file;
    char *file_name;
    char *am_file_name;
    AssemblerContext current_context;
    ext_ptr ext_list_head;
    symbol_ptr temp_sym;
    ext_ptr temp_ext;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        ext_list_head = NULL; /* אתחול רשימת ה-externals עבור כל קובץ מחדש */

        file_name = create_file_name(argv[i], ".as");
        source_file = fopen(file_name, "r");

        if (source_file == NULL) {
            fprintf(stderr, "Error: Cannot open file '%s'. Skipping to next.\n", file_name);
            free(file_name);
            continue;
        }

        /* אתחול ההקשר (Context) עבור הקובץ הנוכחי */
        current_context.ic = INITIAL_IC;
        current_context.dc = 0;
        current_context.line_number = 1;
        current_context.error_found = FALSE;
        current_context.symbol_head = NULL;

        printf("\n--- Processing file: %s.as ---\n", argv[i]);
        printf("Step 1: Processing macros...\n");

        if (pre_assemble(source_file, argv[i], &current_context)) {
            printf("Success! Generated %s.am\n", argv[i]);

            am_file_name = create_file_name(argv[i], ".am");
            am_file = fopen(am_file_name, "r");

            if (am_file != NULL) {
                printf("Step 2: Starting first pass...\n");

                if (first_pass(am_file, &current_context)) {
                    printf("First pass completed successfully!\n");
                    rewind(am_file); /* איפוס סמן הקובץ לקראת המעבר השני */

                    printf("Step 3: Starting second pass...\n");
                    if (second_pass(am_file, &current_context, &ext_list_head)) {
                        printf("Second pass completed successfully!\n");

                        printf("Step 4: Generating output files...\n");
                        generate_output_files(argv[i], &current_context, ext_list_head);
                        printf(">>> Finished processing %s successfully!\n", argv[i]);
                    } else {
                        printf("Failed: Errors found during second pass.\n");
                    }
                } else {
                    printf("Failed: Errors found during first pass.\n");
                }
                fclose(am_file);
            } else {
                fprintf(stderr, "Error: Cannot open %s.am for reading.\n", argv[i]);
            }
            free(am_file_name);
        } else {
            printf("Failed: Errors found during macro expansion in %s\n", file_name);
        }

        fclose(source_file);
        free(file_name);

        /* שלב אחרון: ניקוי ושחרור זיכרון דינאמי לקראת הקובץ הבא (מונע זליגות זיכרון) */
        while (current_context.symbol_head != NULL) {
            temp_sym = current_context.symbol_head;
            current_context.symbol_head = current_context.symbol_head->next;
            free(temp_sym);
        }
        while (ext_list_head != NULL) {
            temp_ext = ext_list_head;
            ext_list_head = ext_list_head->next;
            free(temp_ext);
        }
    }

    return EXIT_SUCCESS;
}
