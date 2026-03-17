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
    AssemblerContext ctx;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 file2 ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        char *as_name = create_file_name(argv[i], ".as");
        char *am_name = create_file_name(argv[i], ".am");
        FILE *as_file = fopen(as_name, "r");
        FILE *am_file;
        ext_ptr ext_list = NULL;

        if (!as_file) {
            fprintf(stderr, "Error: Cannot open %s\n", as_name);
            free(as_name); free(am_name);
            continue;
        }

        /* אתחול ה-Context לכל קובץ מחדש */
        ctx.ic = INITIAL_IC; ctx.dc = 0; ctx.line_number = 1;
        ctx.error_found = FALSE; ctx.symbol_head = NULL;

        printf("\n--- Processing %s ---\n", argv[i]);

        if (pre_assemble(as_file, argv[i], &ctx)) {
            am_file = fopen(am_name, "r");
            if (am_file && first_pass(am_file, &ctx)) {
                rewind(am_file);
                if (second_pass(am_file, &ctx, &ext_list)) {
                    generate_output_files(argv[i], &ctx, ext_list);
                    printf(">>> Done! Output files generated.\n");
                }
            }
            if (am_file) fclose(am_file);
        }

        /* שחרור זיכרון */
        /* כאן יש לקרוא לפונקציות שמשחררות את ctx.symbol_head ואת ext_list */
        fclose(as_file);
        free(as_name); free(am_name);
    }
    return EXIT_SUCCESS;
}
