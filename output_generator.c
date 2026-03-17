/*
 * קובץ: output_generator.c
 * מטרת הקובץ: יצירת קובצי הפלט של האסמבלר (.ob, .ent, .ext)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output_generator.h"
#include "utils.h"

void generate_output_files(const char *base_file_name, AssemblerContext *context, ext_ptr ext_list_head) {
    /* הצהרת כל המשתנים בתחילת הבלוק (C90) */
    char *ob_name;
    char *ent_name;
    char *ext_name;
    FILE *ob_file;
    FILE *ent_file;
    FILE *ext_file;
    int i;
    symbol_ptr sym;
    ext_ptr ext;
    boolean has_entry = FALSE;

    /* 1. בדיקה מקדימה: האם יש בכלל סמלים שהוגדרו כ-entry? */
    sym = context->symbol_head;
    while (sym) {
        if (sym->is_entry) {
            has_entry = TRUE;
            break;
        }
        sym = sym->next;
    }

    /* 2. יצירת קובץ Object (.ob) */
    /* קובץ זה נוצר תמיד אם ההרכבה הצליחה */
    ob_name = create_file_name(base_file_name, ".ob");
    ob_file = fopen(ob_name, "w");
    if (ob_file) {
        /* הדפסת הכותרת: כמות מילות ההוראה וכמות מילות הנתונים */
        fprintf(ob_file, "  %d %d\n", context->ic - INITIAL_IC, context->dc);

        /* הדפסת מערך ההוראות (code_image) */
        for (i = 0; i < context->ic - INITIAL_IC; i++) {
            /* הערה: תבנית ההדפסה כאן היא אוקטלית (בסיס 8) של 5 ספרות.
             * יש להתאים את ה-formatting בדיוק לבסיס שנדרש בחוברת ממן 14 שלכם (למשל בסיס מיוחד, הקסדצימלי וכד') */
            fprintf(ob_file, "%04d %05o\n", INITIAL_IC + i, context->code_image[i].value);
        }

        /* הדפסת מערך הנתונים (data_image) - הכתובת ממשיכה מהנקודה שבה עצר ה-IC */
        for (i = 0; i < context->dc; i++) {
            fprintf(ob_file, "%04d %05o\n", context->ic + i, context->data_image[i].value);
        }
        fclose(ob_file);
    } else {
        fprintf(stderr, "Error: Could not create object file %s\n", ob_name);
    }
    free(ob_name);

    /* 3. יצירת קובץ Entries (.ent) - רק אם יש צורך */
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

    /* 4. יצירת קובץ Externals (.ext) - רק אם רשימת ה-ext לא ריקה */
    if (ext_list_head != NULL) {
        ext_name = create_file_name(base_file_name, ".ext");
        ext_file = fopen(ext_name, "w");
        if (ext_file) {
            ext = ext_list_head;
            while (ext) {
                /* מדפיס את שם הסמל החיצוני ואת הכתובת שבה נעשה בו שימוש */
                fprintf(ext_file, "%s %04d\n", ext->name, ext->address);
                ext = ext->next;
            }
            fclose(ext_file);
        }
        free(ext_name);
    }
}
