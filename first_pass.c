/*
 * קובץ: first_pass.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

/* ================== פונקציות עזר פנימיות (Private) ================== */

static void report_error(AssemblerContext *ctx, const char *message) {
    fprintf(stderr, "Error in line %d: %s\n", ctx->line_number, message);
    ctx->error_found = TRUE;
}

/* פונקציית דמה להוספת סמל לטבלה. עליך להשלים את הלוגיקה האמיתית שלה ב- symbol_table.c */
static boolean add_symbol(AssemblerContext *ctx, const char *name, int value, 
                          boolean is_code, boolean is_data, boolean is_extern) {
    /* TODO: לחפש אם הסמל קיים. אם כן -> להחזיר FALSE. 
       אם לא -> להקצות צומת חדש, לאתחל, ולהוסיף ל- ctx->symbol_head ולהחזיר TRUE. */
    return TRUE; 
}

/* ================== הפונקציה הראשית של המעבר ================== */

boolean run_first_pass(const char *filename, AssemblerContext *ctx) {
    FILE *file;
    char line[MAX_LINE_LENGTH + 2];
    char *ptr;
    char first_word[MAX_LINE_LENGTH + 2];
    char label_name[MAX_LABEL_LENGTH + 2];
    boolean has_label;
    const command_entry *cmd;
    
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Fatal Error: Cannot open file %s for first pass.\n", filename);
        return FALSE;
    }

    /* 1. אתחול מונים (לפי עמוד 48) */
    ctx->ic = INITIAL_IC;
    ctx->dc = 0;
    ctx->line_number = 1;
    ctx->error_found = FALSE;

    /* 2. קרא את השורה הבאה מקובץ המקור */
    while (fgets(line, sizeof(line), file) != NULL) {
        ptr = line;
        has_label = FALSE;
        label_name[0] = '\0';

        /* דילוג על שורות ריקות והערות */
        if (is_empty_or_comment(ptr)) {
            ctx->line_number++;
            continue;
        }

        extract_word(&ptr, first_word);

        /* 3+4. האם השדה הראשון הוא תווית? */
        if (is_label_definition(first_word)) {
            has_label = TRUE;
            /* הסרת הנקודתיים מהשם ושמירה */
            strncpy(label_name, first_word, strlen(first_word) - 1);
            label_name[strlen(first_word) - 1] = '\0';
            
            /* מעבר למילה הבאה בשורה */
            extract_word(&ptr, first_word);
        }

        /* 5. האם זו הנחיה לאחסון נתונים (.data או .string)? */
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            /* 6. אם יש הגדרת סמל, הכנס לטבלה עם מאפיין data וערך DC */
            if (has_label) {
                if (!add_symbol(ctx, label_name, ctx->dc, FALSE, TRUE, FALSE)) {
                    report_error(ctx, "Label already defined");
                }
            }
            
            /* 7. זיהוי נתונים, קידוד לתמונת הנתונים וקידום DC */
            if (strcmp(first_word, ".data") == 0) {
                /* TODO: קרא מספרים מופרדים בפסיקים, הכנס ל- data_image, קדם dc */
            } else {
                /* TODO: קרא מחרוזת, הכנס תווים ל- data_image, הוסף 0 בסוף, קדם dc */
            }
        }
        /* 8. האם זו הנחיית .extern או .entry? */
        else if (strcmp(first_word, ".extern") == 0 || strcmp(first_word, ".entry") == 0) {
            /* 9. אם זו .entry, היא תטופל במעבר השני */
            if (strcmp(first_word, ".extern") == 0) {
                char ext_label[MAX_LABEL_LENGTH + 2];
                extract_word(&ptr, ext_label);
                
                if (!is_valid_label_name(ext_label)) {
                    report_error(ctx, "Invalid extern label name");
                } else {
                    /* 10. הכנס לטבלת סמלים עם ערך 0 ומאפיין extern */
                    if (!add_symbol(ctx, ext_label, 0, FALSE, FALSE, TRUE)) {
                        report_error(ctx, "Extern label already defined");
                    }
                }
            }
            /* אזהרה אם הגדירו תווית לפני הנחיית extern/entry (לפי ההנחיות מותר להתעלם) */
            if (has_label) {
                printf("Warning in line %d: Label before extern/entry is ignored.\n", ctx->line_number);
            }
        }
        /* 11. זוהי שורת הוראה (Instruction) */
        else {
            if (has_label) {
                /* הכנס לטבלה עם מאפיין code וערך IC */
                if (!add_symbol(ctx, label_name, ctx->ic, TRUE, FALSE, FALSE)) {
                    report_error(ctx, "Label already defined");
                }
            }
            
            /* 12. חיפוש שם הפעולה בטבלה */
            cmd = get_command(first_word);
            if (cmd == NULL) {
                report_error(ctx, "Unrecognized instruction name");
            } else {
                /* 13-16. ניתוח אופרנדים, קידוד מילה ראשונה, וחישוב אורך ההוראה (L) */
                int L = 1; /* כל הוראה תופסת לפחות מילה אחת */
                
                /* TODO: פונקציה שתנתח את שארית השורה (ptr) לפי cmd->expected_operands.
                   לכל אופרנד שיימצא, יש להוסיף 1 ל-L (להקצות לו מילת מידע נוספת).
                   בסיום הניתוח: ctx->ic += L; */
                   
                ctx->ic += L; /* עדכון זמני עד למימוש ניתוח האופרנדים */
            }
        }
        ctx->line_number++;
    }

    /* 17. סיום קריאת הקובץ */
    fclose(file);
    if (ctx->error_found) {
        return FALSE;
    }

    /* 18+19. עדכון כתובות של סמלי הנתונים (הוספת ICF לכל סמל Data) */
    /* TODO: לרוץ על ctx->symbol_head ולכל סמל שמוגדר כ-is_data, לעשות: node->value += ctx->ic; */

    return TRUE;
}
