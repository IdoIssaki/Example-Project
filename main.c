#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "utils.h"
#include "pre_assembler.h"
#include "first_pass.h"
#include "second_pass.h"
#include "output_generator.h"

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
        /* יצירת שם הקובץ .as ופתיחתו */
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

        /* שלב 1: פרישת מקרואים (קדם-אסמבלר) */
        if (pre_assemble(source_file, argv[i], &current_context)) {
            char *am_file_name;
            FILE *am_file;
            ext_ptr ext_list_head = NULL;

            printf("Success! Generated %s.am\n", argv[i]);

            /* שלב 2: פתיחת קובץ ה-.am שנוצר לצורך המעברים */
            am_file_name = create_file_name(argv[i], ".am");
            am_file = fopen(am_file_name, "r");

            if (am_file != NULL) {
                printf("Step 2: Starting first pass...\n");

                /* מעבר ראשון */
                if (first_pass(am_file, &current_context)) {
                    printf("First pass completed successfully!\n");

                    /* חובה: החזרת סמן הקובץ להתחלה לפני המעבר השני */
                    rewind(am_file);

                    printf("Step 3: Starting second pass...\n");

                    /* מעבר שני - השלמת תוויות ו-External */
                    if (second_pass(am_file, &current_context, &ext_list_head)) {
                        printf("Second pass completed successfully!\n");

                        /* שלב 4 והאחרון: ייצור קובצי הפלט (.ob, .ent, .ext) */
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
            /* כאן כדאי להוסיף פונקציה לשחרור טבלת הסמלים בסיום הקובץ */

        } else {
            printf("Failed: Errors found during macro expansion in %s\n", file_name);
        }

        fclose(source_file);
        free(file_name);
    }

    return EXIT_SUCCESS;
}
