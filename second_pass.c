/*
 * קובץ: second_pass.c
 */
#include <stdio.h>
#include <string.h>
#include "second_pass.h"
#include "parser.h"
#include "utils.h"

boolean run_second_pass(const char *filename, AssemblerContext *ctx) {
    FILE *file;
    char line[MAX_LINE_LENGTH + 2];
    char *ptr;
    char first_word[MAX_LINE_LENGTH + 2];

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Fatal Error: Cannot open file %s for second pass.\n", filename);
        return FALSE;
    }

    /* 1. אתחול מונה ההוראות לכתובת ההתחלתית (לפי עמוד 50 בחוברת) */
    ctx->ic = INITIAL_IC; /* מתחיל ב-100 */
    ctx->line_number = 1;

    /* 2. קרא את השורה הבאה מקובץ המקור */
    while (fgets(line, sizeof(line), file) != NULL) {
        ptr = line;

        /* דילוג על שורות ריקות והערות */
        if (is_empty_or_comment(ptr)) {
            ctx->line_number++;
            continue;
        }

        extract_word(&ptr, first_word);

        /* 3. האם המילה הראשונה היא תווית? */
        if (is_label_definition(first_word)) {
            /* במעבר השני אנחנו מתעלמים מהגדרות של תוויות (כבר הוספנו אותן במעבר הראשון) */
            extract_word(&ptr, first_word);
        }

        /* 4. האם זו הנחיית נתונים (.data או .string) או .extern? */
        if (strcmp(first_word, ".data") == 0 || 
            strcmp(first_word, ".string") == 0 || 
            strcmp(first_word, ".extern") == 0) {
            /* 5. מתעלמים! כבר טיפלנו בהם במעבר הראשון */
            ctx->line_number++;
            continue;
        }

        /* 6. האם זו הנחיית .entry? */
        if (strcmp(first_word, ".entry") == 0) {
            char entry_label[MAX_LABEL_LENGTH + 2];
            extract_word(&ptr, entry_label);
            
            /* 7. מסמנים את התווית בטבלת הסמלים כ-entry */
            /* TODO: 
             * 1. הפעל פונקציה: symbol = find_symbol(ctx->symbol_head, entry_label);
             * 2. אם הפונקציה החזירה NULL (התווית לא קיימת) -> דווח על שגיאה (ctx->error_found = TRUE).
             * 3. אם קיימת -> שנה את המאפיין: symbol->is_entry = TRUE;
             */
        }
        /* 8. אם הגענו לכאן - זוהי פקודת הוראה (Instruction) */
        else {
            /* 9. השלמת הקידוד לתמונת ההוראות */
            /* TODO: 
             * כאן צריך לנתח את האופרנדים שוב. 
             * - אם האופרנד הוא מספר (מיעון מיידי) או רגיסטר -> כבר קודדנו אותו במעבר 1.
             * - אם האופרנד הוא *תווית* (מיעון ישיר או יחסי):
             * 1. חפש את התווית בטבלת הסמלים (find_symbol).
             * 2. אם היא לא שם -> שגיאה (שימוש בתווית שלא הוגדרה).
             * 3. אם היא שם, קח את הערך שלה (symbol->value) וקודד אותו למילה הריקה שהשארנו ב- instructions_image[ic].
             * 4. אם התווית מוגדרת כ-Extern -> הוסף רשומה חדשה לרשימת ה-externals שלך (כדי שאחר כך תדפיס אותה לקובץ .ext).
             *
             * לבסוף, קדם את ה-IC בהתאם לכמות מילות הזיכרון (L) שהפקודה תופסת.
             */
            int L = 1; /* אורך מינימלי. יש לעדכן לפי ניתוח האופרנדים */
            ctx->ic += L;
        }

        ctx->line_number++;
    }

    fclose(file);

    /* 10. האם נמצאו שגיאות (במעבר הראשון או השני)? */
    if (ctx->error_found) {
        /* עוצרים כאן ולא מייצרים את קבצי הפלט! */
        return FALSE;
    }

    /* 11. הכל עבר בהצלחה! יצירת קבצי הפלט (השלב הסופי בהחלט) */
    /* TODO: 
     * כאן תקרא ל-3 פונקציות (לרוב נהוג לשים אותן בקובץ outputs.c):
     * 1. generate_ob_file(ctx, filename);  -> מדפיס את תמונות הזיכרון.
     * 2. generate_ent_file(ctx, filename); -> מדפיס רק סמלים שהם is_entry == TRUE.
     * 3. generate_ext_file(ctx, filename); -> מדפיס את רשימת השימושים ב-extern.
     */

    return TRUE;
}
