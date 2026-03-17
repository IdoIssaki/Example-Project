#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output_generator.h"
#include "utils.h"

/* פונקציית עזר להדפסת מילה בבסיס 8 (אוקטלי) עם 4 ספרות */
static void print_word_octal(FILE *file, int value) {
    /* מיסוך ל-12 ביטים כדי להבטיח שהמספר חיובי בטווח הנכון */
    fprintf(file, "%04o\n", value & 0xFFF);
}

void generate_output_files(const char *base_file_name, AssemblerContext *context, ext_ptr ext_list_head) {
    char *ob_name, *ent_name, *ext_name;
    FILE *ob_file, *ent_file, *ext_file;
    int i;
    symbol_ptr sym;
    boolean has_entry = FALSE;

    /* --- 1. יצירת קובץ .ob (Object) --- */
    ob_name = create_file_name(base_file_name, ".ob");
    ob_file = fopen(ob_name, "w");
    if (ob_file) {
        /* הדפסת כותרת: כמות מילות הוראה וכמות מילות נתונים */
        fprintf(ob_file, "  %d %d\n", context->ic - INITIAL_IC, context->dc);

        /* הדפסת תמונת הקוד */
        for (i = 0; i < context->ic - INITIAL_IC; i++) {
            fprintf(ob_file, "%04d ", INITIAL_IC + i);
            print_word_octal(ob_file, context->code_image[i].value);
        }

        /* הדפסת תמונת הנתונים */
        for (i = 0; i < context->dc; i++) {
            fprintf(ob_file, "%04d ", context->ic + i);
            print_word_octal(ob_file, context->data_image[i].value);
        }
        fclose(ob_file);
    }
    free(ob_name);

    /* --- 2. יצירת קובץ .ent (Entries) --- */
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

    /* --- 3. יצירת קובץ .ext (Externals) --- */
    if (ext_list_head != NULL) {
        ext_name = create_file_name(base_file_name, ".ext");
        ext_file = fopen(ext_name, "w");
        if (ext_file) {
            ext_ptr curr = ext_list_head;
            while (curr) {
                fprintf(ext_file, "%s %04d\n", curr->name, curr->address);
                curr = curr->next;
            }
            fclose(ext_file);
        }
        free(ext_name);
    }
}


