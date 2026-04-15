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

    // וויאוי שהמשתמש הזין לפחות שם של קובץ אחד לעיבוד.
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file1 file2 ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    // המשתנים צריכים להיות מוכרזים בהתחלה.
    // יוצרים את קובץ as וקובץ am ופותחים את as לקריאה.
    for (i = 1; i < argc; i++) {
        char *as_name = create_file_name(argv[i], ".as");
        char *am_name = create_file_name(argv[i], ".am");
        FILE *as_file = fopen(as_name, "r");
        /* אתחול ל-NULL למניעת שימוש בערכי זבל במקרה של שגיאה */
        FILE *am_file = NULL;
        ext_ptr ext_list = NULL;

        if (!as_file) {
            fprintf(stderr, "Error: Cannot open %s\n", as_name);
            free(as_name);
            free(am_name);
            continue;
        }

        /* אתחול ה-Context לכל קובץ מחדש */
        ctx.ic = INITIAL_IC;
        ctx.dc = 0;
        ctx.line_number = 1;
        ctx.error_found = FALSE;
        ctx.symbol_head = NULL;

        printf("\n--- Processing %s ---\n", argv[i]);

        /* שלב 1: פריסת מאקרויים (Pre-Assembler) */
        if (pre_assemble(as_file, argv[i], &ctx)) {
            am_file = fopen(am_name, "r"); // מחזיר אוטומטית את הסמן למילה הראשונה בקובץ. עושה rewind.

            /* שלב 2: מעבר ראשון */
            if (am_file && first_pass(am_file, &ctx)) {
                rewind(am_file); /* חזרה לתחילת קובץ ה-am עבור המעבר השני */

                /* שלב 3: מעבר שני */
                // אין פה צורך לרשום את התנאי השני לפני הוגם כי בדקנו כבר במעבר הראשון שאנחנו בקובץ הזה.
                if (second_pass(am_file, &ctx, &ext_list)) {
                    /* שלב 4: יצירת קבצי הפלט */
                    generate_output_files(argv[i], &ctx, ext_list);
                    printf(">>> Done! Output files generated for %s.\n", argv[i]);
                }
            }
            if (am_file) fclose(am_file);
        }

        /* סגירת קובץ המקור */
        fclose(as_file);

        /* --- שחרור זיכרון יסודי לפני מעבר לקובץ הבא --- */
        free(as_name);
        free(am_name);

        /* שחרור טבלת הסמלים (הפונקציה נמצאת ב-utils.c) */
        free_symbols(ctx.symbol_head);

        /* שחרור רשימת האקסטרנים (לולאת שחרור ישירה ב-main) */
        {
            ext_ptr temp_ext;
            while (ext_list != NULL) {
                temp_ext = ext_list;
                ext_list = ext_list->next;
                free(temp_ext);
            }
        }

    } /* סוף לולאת מעבר על הקבצים */

    return EXIT_SUCCESS;
}
