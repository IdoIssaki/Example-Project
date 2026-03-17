/*
 * קובץ: parser.c
 */
 #include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

boolean parse_string_directive(char **line_ptr, AssemblerContext *ctx) {
    char *ptr = *line_ptr;
    
    skip_whitespaces(&ptr);

    /* בדיקה האם המחרוזת מתחילה במרכאות */
    if (*ptr != '"') {
        return FALSE; 
    }
    ptr++; 

    /* סריקת התווים עד לסגירת המרכאות או סוף השורה */
    while (*ptr != '\0' && *ptr != '"') {
        /* תיקון: גישה לשדות ה-struct של מילת המכונה */
        ctx->data_image[ctx->dc].value = (unsigned int)(*ptr);
        ctx->data_image[ctx->dc].are = ARE_ABSOLUTE; /* נתונים הם תמיד Absolute */
        ctx->dc++;
        ptr++;
    }

    if (*ptr != '"') {
        return FALSE; 
    }
    ptr++; 

    /* הוספת תו סיום מחרוזת (0) חובה לפי ההנחיות! */
    ctx->data_image[ctx->dc].value = 0;
    ctx->data_image[ctx->dc].are = ARE_ABSOLUTE;
    ctx->dc++;

    skip_whitespaces(&ptr);
    if (!is_empty_or_comment(ptr)) {
        return FALSE; 
    }

    *line_ptr = ptr;
    return TRUE;
}

boolean parse_data_directive(char **line_ptr, AssemblerContext *ctx) {
    char *ptr = *line_ptr;
    int value;
    boolean comma_expected = FALSE;
    char *end_ptr;

    skip_whitespaces(&ptr);
    if (is_empty_or_comment(ptr)) {
        return FALSE; 
    }

    while (!is_empty_or_comment(ptr)) {
        skip_whitespaces(&ptr);

        if (*ptr == ',') {
            if (!comma_expected) {
                return FALSE; 
            }
            comma_expected = FALSE;
            ptr++;
            continue;
        }

        if (comma_expected) {
            return FALSE; 
        }

        value = (int)strtol(ptr, &end_ptr, 10);
        
        if (ptr == end_ptr) {
            return FALSE; 
        }

        /* תיקון: גישה לשדות ה-struct */
        ctx->data_image[ctx->dc].value = (unsigned int)value;
        ctx->data_image[ctx->dc].are = ARE_ABSOLUTE; /* נתונים הם תמיד Absolute */
        ctx->dc++;
        
        comma_expected = TRUE; 
        ptr = end_ptr;
    }

    if (!comma_expected) {
        return FALSE; 
    }

    *line_ptr = ptr;
    return TRUE;
}
