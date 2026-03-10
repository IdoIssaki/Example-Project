/*
 * קובץ: main.c (מיועד לבדיקת שלב הקדם-אסמבלר בלבד)
 */
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "utils.h"
#include "pre_assembler.h"
#include "first_pass.h"

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
        
        if (pre_assemble(source_file, argv[i], &current_context)) {
            char am_file_name[256];
            printf("Success! Generated %s.am\n", argv[i]);
            
            /* Prepare the filename with the .am extension for the first pass */
            sprintf(am_file_name, "%s.am", argv[i]);

            /* Execute the first pass */
            if (execute_first_pass(am_file_name, &current_context)) {
                printf("First pass completed successfully for %s\n", am_file_name);
                
                /* The second pass will be called here in the future */
            } else {
                printf("Errors found during the first pass in %s\n", am_file_name);
            }
        } else {
            printf("Failed: Errors found in %s\n", file_name);
        }

        fclose(source_file);
        free(file_name);
    }

    return EXIT_SUCCESS;
}
