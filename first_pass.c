#include <stdio.h>
#include <string.h>
#include "first_pass.h"

/* Assumption: the initialization function we created is in utils.h */
#include "utils.h" 

boolean execute_first_pass(const char *am_file_name, AssemblerContext *context) {
    FILE *am_file;
    char line[MAX_LINE_LENGTH + 2]; /* +2 to leave room for the newline character and \0 */

    /* 1. Attempt to open the .am file for reading */
    am_file = fopen(am_file_name, "r");
    if (am_file == NULL) {
        printf("Error: Could not open file %s for the first pass.\n", am_file_name);
        return FALSE;
    }

    /* 2. Initialize the context (IC, DC, error flags) before starting */
    init_context(context);

    /* 3. Read the file line by line */
    while (fgets(line, sizeof(line), am_file) != NULL) {
        
        /* Parser logic will be implemented here later:
         * - Skip empty lines and comments
         * - Identify labels
         * - Increment IC and DC counters
         */

        /* Increment the line counter for accurate error reporting */
        context->current_line++;
    }

    /* 4. Close the file */
    fclose(am_file);
    
    /* Return FALSE if any errors were found during the pass, TRUE otherwise */
    return !(context->has_errors);
}
