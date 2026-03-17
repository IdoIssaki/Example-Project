#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output_generator.h"
#include "utils.h"

/* הנה פונקציית העזר! כאן השורה המיוחדת אמורה להיות. */
static void print_machine_word(FILE *file, int address, machine_word word) {
    fprintf(file, "%04d %03X %c\n", address, word.value & 0xFFF, word.are);
}

void generate_output_files(const char *base_file_name, AssemblerContext *context, ext_ptr ext_list_head) {
    char *ob_name, *ent_name, *ext_name;
    FILE *ob_file, *ent_file, *ext_file;
    int i;
    symbol_ptr sym;
    ext_ptr curr_ext;
    boolean has_entry = FALSE;

    ob_name = create_file_name(base_file_name, ".ob");
    ob_file = fopen(ob_name, "w");
    if (ob_file) {
        fprintf(ob_file, "  %d %d\n", context->ic - INITIAL_IC, context->dc);

        for (i = 0; i < context->ic - INITIAL_IC; i++) {
            print_machine_word(ob_file, INITIAL_IC + i, context->code_image[i]);
        }

        for (i = 0; i < context->dc; i++) {
            print_machine_word(ob_file, context->ic + i, context->data_image[i]);
        }
        fclose(ob_file);
    }
    free(ob_name);

    sym = context->symbol_head;
    while (sym) {
        if (sym->is_entry) { has_entry = TRUE; break; }
        sym = sym->next;
    }

    if (has_entry) {
        ent_name = create_file_name(base_file_name, ".ent");
        ent_file = fopen(ent_name, "w");
        if (ent_file) {
            sym = context->symbol_head;
            while (sym) {
                if (sym->is_entry) {
                    fprintf(ent_file, "%s %04d\n", sym->name, sym->value);
                }
                sym = sym->next;
            }
            fclose(ent_file);
        }
        free(ent_name);
    }

    if (ext_list_head != NULL) {
        ext_name = create_file_name(base_file_name, ".ext");
        ext_file = fopen(ext_name, "w");
        if (ext_file) {
            curr_ext = ext_list_head;
            while (curr_ext) {
                fprintf(ext_file, "%s %04d\n", curr_ext->name, curr_ext->address);
                curr_ext = curr_ext->next;
            }
            fclose(ext_file);
        }
        free(ext_name);
    }
}
