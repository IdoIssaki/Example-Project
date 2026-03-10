#include <stdio.h>
#include <string.h>
#include "first_pass.h"
#include "utils.h"
#include "parser.h"

boolean execute_first_pass(const char *am_file_name, AssemblerContext *context) {
    FILE *am_file;
    char line[MAX_LINE_LENGTH + 2]; /* +2 to leave room for the newline character and \0 */

    /* 1. Attempt to open the .am file for reading */
    am_file = fopen(am_file_name, "r");
    if (am_file == NULL) {
        printf("Error: Could not open file %s for the first pass.\n", am_file_name);
        return FALSE;
    }

    /* 2. Initialize the context directly here */
    context->ic = 100;              /* Instructions start at memory address 100 */
    context->dc = 0;                /* Data starts at 0 */
    context->line_number = 1;       /* Start reading from line 1 */
    context->error_found = FALSE;   /* No errors initially */
    context->symbol_head = NULL;    /* Empty symbol table */

    /* 3. Read the file line by line */
    while (fgets(line, sizeof(line), am_file) != NULL) {
        char *ptr = line; /* Pointer to traverse the line */
        
        /* DECLARATIONS MUST BE AT THE TOP OF THE BLOCK IN C90 */
        char first_word[MAX_LINE_LENGTH];
        boolean is_label = FALSE;
        char label_name[MAX_LINE_LENGTH];

        /* Skip leading white spaces (spaces and tabs) */
        while (*ptr == ' ' || *ptr == '\t') {
            ptr++;
        }

        /* Check if the line is empty, just a newline, or a comment */
        if (*ptr == '\0' || *ptr == '\n' || *ptr == ';') {
            context->line_number++;
            continue; /* Move directly to the next line */
        }

        /* 1. Extract the first word from the line using the parser function */
        extract_word(&ptr, first_word);

        /* 2. Check if the word is a label definition (e.g., "MAIN:") */
        if (is_label_definition(first_word)) {
            is_label = TRUE;
            
            /* Save the label name without the colon at the end */
            strncpy(label_name, first_word, strlen(first_word) - 1);
            label_name[strlen(first_word) - 1] = '\0';
            
            /* Skip white spaces after the label to reach the actual instruction */
            while (*ptr == ' ' || *ptr == '\t') {
                ptr++;
            }
            
            /* Extract the next word (which should now be the instruction/directive) */
            if (*ptr != '\0' && *ptr != '\n' && *ptr != ';') {
                extract_word(&ptr, first_word);
            } else {
                /* If there is only a label on the line, it is an error */
                printf("Error in line %d: Label without instruction.\n", context->line_number);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
        }

/* Classify the instruction/directive */
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            if (is_label) {
                /* * We found a label before a data directive.
                 * Later we will add it to the symbol table with the value of DC.
                 */
                printf("Found data label: %s at DC: %d\n", label_name, context->dc);
            }
            
            /* * TODO: Parse the data/string arguments, store them in data_image, 
             * and increment DC according to the data size.
             */
            
        } else if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            /* Handling for .extern and .entry directives will go here */
            if (is_label) {
                printf("Warning in line %d: Label before %s is ignored.\n", context->line_number, first_word);
            }
            
        } else {
            /* If it's not a directive, it must be a code instruction (e.g., mov, add) */
            if (is_label) {
                /* * We found a label before a code instruction.
                 * Later we will add it to the symbol table with the value of IC.
                 */
                printf("Found code label: %s at IC: %d\n", label_name, context->ic);
            }
            
            /* * TODO: Identify the instruction, calculate its length (L), 
             * and increment IC by L.
             */
        }

        /* Increment the line counter */
        context->line_number++;
    } /* End of while loop */

    /* 4. Close the file */
    fclose(am_file);
    
    /* Return FALSE if any errors were found, TRUE otherwise */
    return !(context->error_found);
}
