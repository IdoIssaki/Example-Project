#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output_generator.h"
#include "utils.h"

/* מדפיס מילת מכונה לקובץ */
// %04d דואג להדפסת כתובת הזיכרון(מספר השורה) ב-4 ספרות עשרוניות
// %03X דואג להדפסת תוכן המילה ב-3 ספרות הקסה דצמליות.
// דואג לקחת את 12 הביטים הרלוונטים של המילה word.value & 0xFFF
static void print_machine_word(FILE *file, int address, machine_word word) {
    fprintf(file, "%04d %03X %c\n", address, word.value & 0xFFF, word.are);
}

//
void generate_output_files(const char *base_file_name, AssemblerContext *context, ext_ptr ext_list_head) {
    char *ob_name, *ent_name, *ext_name;
    FILE *ob_file, *ent_file, *ext_file;
    int i;
    symbol_ptr sym;
    ext_ptr curr_ext;
    boolean has_entry = FALSE;

    //יצירת קובץ ob וכתיבה בתוכו
    ob_name = create_file_name(base_file_name, ".ob");
    ob_file = fopen(ob_name, "w");
    if (ob_file) {
        //הדפסת השורה הראשונה שמתארת את מספר שורות הפקודות ומספר שורות הנתונים בסך כל הקובץ.
        fprintf(ob_file, "  %d %d\n", context->ic - INITIAL_IC, context->dc);

        //הדפסת השורות של הקוד הנמצאות בטבלת הפקודות
        for (i = 0; i < context->ic - INITIAL_IC; i++) {
            print_machine_word(ob_file, INITIAL_IC + i, context->code_image[i]);
        }

        //הדפסת השורות של הקוד הנמצאות בטבלת הנתונים
        for (i = 0; i < context->dc; i++) {
            print_machine_word(ob_file, context->ic + i, context->data_image[i]);
        }

        //סגירת הקובץ
        fclose(ob_file);
    }

    //שחרור זיכרון הקובץ
    free(ob_name);

    //בדיקה עבור תווית אם היא entry
    sym = context->symbol_head;
    while (sym) {
        if (sym->is_entry) { has_entry = TRUE; break; }
        sym = sym->next;
    }

    // אם היא כן, ניצור את קובץ entry ונכתוב בו
    if (has_entry) {
        ent_name = create_file_name(base_file_name, ".ent");
        ent_file = fopen(ent_name, "w");
        if (ent_file) {
            sym = context->symbol_head;
            while (sym) {
                // הדפסת תוכן השורה של התווית וקידום לבדיקת התווית הבאה בטבלה (בודקים אם היא entry).
                if (sym->is_entry) {
                    fprintf(ent_file, "%s %04d\n", sym->name, sym->value);
                }
                sym = sym->next;
            }
            //סגירת הקובץ
            fclose(ent_file);
        }
        //שחרור זיכרון הקובץ
        free(ent_name);
    }

    // אם ברשימת תוויות ה-extern יש תוויות, ניצור קובץ extern ונכתוב בו
    if (ext_list_head != NULL) {
        ext_name = create_file_name(base_file_name, ".ext");
        ext_file = fopen(ext_name, "w");
        if (ext_file) {
            curr_ext = ext_list_head;
            while (curr_ext) {
                // הדפסת תוכן השורה של התווית וקידום לבדיקת התווית הבאה בטבלה (בודקים אם היא extern).
                fprintf(ext_file, "%s %04d\n", curr_ext->name, curr_ext->address);
                curr_ext = curr_ext->next;
            }
            //סגירת הקובץ
            fclose(ext_file);
        }
        //שחרור זיכרון הקובץ
        free(ext_name);
    }
}
