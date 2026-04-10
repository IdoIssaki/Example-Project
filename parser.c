/*
 * קובץ: parser.c
 */
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "tables.h"
#include "utils.h"

boolean is_reserved_word(const char *word) {
    if (word == NULL) return FALSE;
    if (is_register(word) || get_command(word) != NULL) return TRUE;
    if (strcmp(word, ".data") == 0   || strcmp(word, ".string") == 0 ||
        strcmp(word, ".entry") == 0  || strcmp(word, ".extern") == 0 ||
        strcmp(word, "mcro") == 0    || strcmp(word, "mcroend") == 0) {
        return TRUE;
    }
    return FALSE;
}

boolean is_valid_label_name(const char *name) {
    int i, length;
    if (name == NULL) return FALSE;
    length = strlen(name);
    if (length == 0 || length > MAX_LABEL_LENGTH) return FALSE;
    if (!isalpha(name[0])) return FALSE;
    for (i = 1; i < length; i++) {
        if (!isalnum(name[i])) return FALSE;
    }
    if (is_reserved_word(name)) return FALSE;
    return TRUE;
}

void extract_word(char **source, char *dest) {
    int i = 0;
    if (source == NULL || *source == NULL || dest == NULL) return;
    skip_whitespaces(source);
    while (**source != '\0' && **source != '\n' && 
           **source != ' ' && **source != '\t' && **source != ',') {
        dest[i] = **source;
        i++;
        (*source)++;
    }
    dest[i] = '\0';
}

boolean is_label_definition(const char *word) {
    int length;
    char name_without_colon[MAX_LABEL_LENGTH + 2];
    if (word == NULL) return FALSE;
    length = strlen(word);
    if (length < 2 || word[length - 1] != ':') return FALSE;
    strncpy(name_without_colon, word, length - 1);
    name_without_colon[length - 1] = '\0';
    return is_valid_label_name(name_without_colon);
}

boolean is_valid_number(const char *str) {
    int i = 0;
    if (str == NULL || str[0] == '\0') return FALSE;
    if (str[0] == '+' || str[0] == '-') {
        i = 1;
        if (str[i] == '\0') return FALSE;
    }
    while (str[i] != '\0') {
        if (!isdigit(str[i])) return FALSE;
        i++;
    }
    return TRUE;
}

boolean is_valid_string_literal(const char *str) {
    int length, i;
    if (str == NULL) return FALSE;
    length = strlen(str);
    if (length < 2 || str[0] != '"' || str[length - 1] != '"') return FALSE;
    for (i = 1; i < length - 1; i++) {
        if (!isprint(str[i])) return FALSE;
    }
    return TRUE;
}

/* בודק אם שיטת מיעון חוקית לפקודה */
// צריך להוסיף אותה בקוד שלנו ובנוסף- להוסיף גם עוד מספר פונקציות כדי להקל על pre assembler (בשיחה עם הגימניי- לבדוק)
//נצטרך גם להכניס פונקציה שתעבוד עם short כפי שג'ודי ביקשה כי כרגע הם לא על short
// יש שורה לא קריאה בfirst pass שהגימנאיי ממליץ במקומה לעשות פה פונקציה נוספת שתקל על הכתיבה שם. (לבדוק)
boolean is_valid_addr_mode(const int modes[], int mode) {
    if (mode < 0 || mode > 3) return FALSE;
    return modes[mode] == 1;
}



//-----------------------------------------------------------------------------------------------------------------
/* פונקציות שלא נעשה להם שימוש ונועדו לחסוך חלק מהקוד בfirst pass */

//זו ספציפית לא חוסכת קוד אלא עושה אשתי שוורת יותר קריאות.
int get_addressing_mode(const char *operand) {
    if (operand == NULL || *operand == '\0') return -1;

    /* שיטה 0: מיידי (מתחיל ב-#) */
    if (operand[0] == '#') return 0;

    /* שיטה 3: אוגר (r0-r7) */
    if (is_register(operand)) return 3;

    /* שיטה 2: יחסי (מתחיל ב-%) */
    if (operand[0] == '%') return 2;

    /* שיטה 1: ישיר (תווית - כל מה שלא עונה על התנאים למעלה) */
    /* הערה: במעבר הראשון נוודא שהשם עצמו חוקי כתווית */
    return 1;
}


boolean parse_data_directive(char **line_ptr, AssemblerContext *ctx) {
    char num_str[MAX_LINE_LENGTH];
    boolean expect_number = TRUE;
    boolean found_at_least_one = FALSE;

    while (**line_ptr != '\0' && **line_ptr != '\n') {
        skip_whitespaces(line_ptr);
        if (**line_ptr == '\0' || **line_ptr == '\n') break;

        /* טיפול בפסיק */
        if (**line_ptr == ',') {
            if (expect_number) {
                fprintf(stderr, "Error line %d: Unexpected comma or multiple commas in .data\n", ctx->line_number);
                ctx->error_found = TRUE;
                return FALSE;
            }
            (*line_ptr)++;
            expect_number = TRUE;
            continue;
        }

        /* אם אנחנו לא מצפים למספר (כלומר, חסר פסיק בין מספרים) */
        if (!expect_number) {
            fprintf(stderr, "Error line %d: Expected comma between numbers in .data\n", ctx->line_number);
            ctx->error_found = TRUE;
            return FALSE;
        }

        /* חילוץ המספר */
        extract_word(line_ptr, num_str);
        if (num_str[0] != '\0') {
            if (!is_valid_number(num_str)) {
                fprintf(stderr, "Error line %d: Invalid number '%s' in .data\n", ctx->line_number, num_str);
                ctx->error_found = TRUE;
                return FALSE;
            }

            /* הכנסה לתמונת הנתונים */
            ctx->data_image[ctx->dc].value = atoi(num_str) & 0xFFF; /* שמירת 12 ביט */
            ctx->data_image[ctx->dc].are = ARE_ABSOLUTE;
            ctx->dc++;

            expect_number = FALSE;
            found_at_least_one = TRUE;
        }
    }

    /* בדיקת שגיאה: האם השורה הסתיימה בפסיק מיותר? */
    if (expect_number && found_at_least_one) {
        fprintf(stderr, "Error line %d: Trailing comma at the end of .data directive\n", ctx->line_number);
        ctx->error_found = TRUE;
        return FALSE;
    }

    return found_at_least_one;
}



boolean parse_string_directive(char **line_ptr, AssemblerContext *ctx) {
    skip_whitespaces(line_ptr);

    /* 1. בדיקה שיש מרכאה פותחת */
    if (**line_ptr != '\"') {
        fprintf(stderr, "Error line %d: Missing opening quote for .string directive\n", ctx->line_number);
        ctx->error_found = TRUE;
        return FALSE;
    }

    (*line_ptr)++; /* מדלגים על המרכאה הפותחת */

    /* 2. הכנסת התווים למערך הנתונים */
    while (**line_ptr != '\"' && **line_ptr != '\0' && **line_ptr != '\n') {
        ctx->data_image[ctx->dc].value = (unsigned int)**line_ptr & 0xFFF;
        ctx->data_image[ctx->dc].are = ARE_ABSOLUTE;
        ctx->dc++;
        (*line_ptr)++;
    }

    /* 3. בדיקה אם עצרנו בגלל מרכאה סוגרת או בגלל שנגמרה השורה */
    if (**line_ptr != '\"') {
        fprintf(stderr, "Error line %d: Missing closing quote for .string directive\n", ctx->line_number);
        ctx->error_found = TRUE;
        return FALSE;
    }

    /* 4. סגירת המחרוזת עם תו ה-NULL (אפס) */
    ctx->data_image[ctx->dc].value = 0;
    ctx->data_image[ctx->dc].are = ARE_ABSOLUTE;
    ctx->dc++;

    (*line_ptr)++; /* מדלגים על המרכאה הסוגרת */

    /* 5. בדיקה שאין טקסט מיותר אחרי המחרוזת */
    skip_whitespaces(line_ptr);
    if (**line_ptr != '\0' && **line_ptr != '\n') {
        fprintf(stderr, "Error line %d: Extraneous text after string in .string directive\n", ctx->line_number);
        ctx->error_found = TRUE;
        return FALSE;
    }

    return TRUE;
}


boolean parse_command_operands(char **line_ptr, char *src, char *dst, int expected_ops, int line_number) {
    int k;
    skip_whitespaces(line_ptr);

    if (expected_ops == 2) {
        /* 1. חילוץ אופרנד מקור */
        k = 0;
        while (**line_ptr && **line_ptr != ',' && !isspace(**line_ptr)) {
            src[k++] = *(*line_ptr)++;
        }
        src[k] = '\0';

        /* 2. טיפול בפסיק */
        skip_whitespaces(line_ptr);
        if (**line_ptr != ',') {
            fprintf(stderr, "Error line %d: Missing comma between operands\n", line_number);
            return FALSE;
        }
        (*line_ptr)++; /* מדלגים על הפסיק */
        skip_whitespaces(line_ptr);

        /* 3. חילוץ אופרנד יעד */
        k = 0;
        while (**line_ptr && **line_ptr != ',' && !isspace(**line_ptr)) {
            dst[k++] = *(*line_ptr)++;
        }
        dst[k] = '\0';

        if (strlen(src) == 0 || strlen(dst) == 0) {
            fprintf(stderr, "Error line %d: Missing operand(s)\n", line_number);
            return FALSE;
        }
    }
    else if (expected_ops == 1) {
        /* חילוץ אופרנד יעד בלבד */
        k = 0;
        while (**line_ptr && **line_ptr != ',' && !isspace(**line_ptr)) {
            dst[k++] = *(*line_ptr)++;
        }
        dst[k] = '\0';

        if (strlen(dst) == 0) {
            fprintf(stderr, "Error line %d: Missing operand\n", line_number);
            return FALSE;
        }
    }

    /* בדיקה שלא נשאר זבל בסוף השורה */
    skip_whitespaces(line_ptr);
    if (**line_ptr != '\0' && **line_ptr != '\n') {
        fprintf(stderr, "Error line %d: Extraneous text after operands\n", line_number);
        return FALSE;
    }

    return TRUE;
}

