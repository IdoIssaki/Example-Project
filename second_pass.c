#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "second_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"
// הערה: הרבה קוד כפול וחוסר עקביוץ- למשל חסר אקסטרה טקסט עבור פקודות עם אופרנד יחיד. (במעבר הראשון גם מופיע משום מה)
//  גימנאיי שלח קוד נקי עבור מעבר ראשון ושני והוסיף פונקציות נוספות בparser שיעזרו עם הקוד הכפול.
//  עוד לא הסתכלתי על מה שהוא שלח - נעבור על זה ביחד.

//-----------------------------------------------------------

//השמת הנתונים והכנסת התווית extern לרשימה.
static void add_ext(ext_ptr *head, const char *name, int address) {
    ext_ptr new_node = (ext_ptr)safe_malloc(sizeof(ext_node));
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->next = NULL;

    // האיבר הראשון ברשימה
    if (*head == NULL) {
        *head = new_node; }

    else {
        ext_ptr temp = *head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = new_node;
    }
}

boolean second_pass(FILE *am_file, AssemblerContext *context, ext_ptr *ext_list_head) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;
    int word_addr;
    char *sym_name;
    symbol_ptr sym;

    context->ic = INITIAL_IC;
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        // עדיף להכריז על המשתנים בתחילת התוכנית.
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH], dst[MAX_LINE_LENGTH];
        int src_m = -1, dst_m = -1, L = 1, offset, k;

        line_ptr = line;

        // אם השורה ריקה או הערה, המשך
        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        // הוצאת המילה הראשונה
        extract_word(&line_ptr, first_word);

        /* בדיקה אבסולוטית בדומה למעבר הראשון */
        // בדיקה אם התווית חוקית- מתחילה עם אות גדולה ולא מילה שמורה.
        // קוד שמופיע גם בfirst pass- עדיף לעשות גם פונקציה שבודקת אם התווית מתחילה באות גדולה
        if (strlen(first_word) > 0 && first_word[strlen(first_word) - 1] == ':') {
            if (!((first_word[0] >= 'A' && first_word[0] <= 'Z') || (first_word[0] >= 'a' && first_word[0] <= 'z'))) {
                fprintf(stderr, "Error line %d: Invalid label '%s' (Label MUST start with a letter)\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            if (!is_label_definition(first_word)) {
                fprintf(stderr, "Error line %d: Invalid label syntax '%s'\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            extract_word(&line_ptr, first_word);
        }


        // בגלל שנבדק כבר במעבר הראשון- מקדמים את השורה וממשיכים
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0 || strcmp(first_word, ".extern") == 0) {
            context->line_number++;
            continue;
        }
        // בדיקה עבור תוויות entry
        else if (strcmp(first_word, ".entry") == 0) {
            // הוצאת המילה הבאה- שם התווית
            extract_word(&line_ptr, label);

            /* בדיקה שתווית האנטרי מתחיל באות- אם לא הדפסה ושגיאה למשתמש, אחרת בודקים עבור טבלת הסמלים */
            if (!((label[0] >= 'A' && label[0] <= 'Z') || (label[0] >= 'a' && label[0] <= 'z'))) {
                fprintf(stderr, "Error line %d: Invalid entry label '%s' (Must start with a letter)\n", context->line_number, label);
                context->error_found = TRUE;
            } else {
                sym = get_symbol(context->symbol_head, label);
                // אם התווית נראתה בטבלת הסמלים עם השם הזה,
                // אם לא זה לא טוב כי הטבלת סמלים כבר מלאה ולא ניתן בשלב המעבר השני להוסיף סמל.
                if (sym) {
                    /* בדיקת התנגשות: האם כבר הוגדר כ-extern במעבר הראשון? */
                    // אם התווית כבר קיימת בטבלה כextern, זו שגיאה כי תווית לא יכולה להיות גם entry וגם extern.
                    if (sym->is_extern) {
                        fprintf(stderr, "Error line %d: Symbol '%s' cannot be defined as both .entry and .extern\n", context->line_number, label);
                        context->error_found = TRUE;
                    } else {
                        //  אם היא לא extern, היא יכולה להיות בוודאות entry כמו שרצינו להגדירה ולכן נסמל בדגל is.entry אמת.
                        sym->is_entry = TRUE;
                    }
                } else {
                    // הגדרנו תווית כentry אבל היא לא בטבלת הסמלים, סימן שהיא לא הוכרזה (עם נקודתיים והכל) כי טבלת הסמלים שלנו כבר מלאה מה-first pass
                    fprintf(stderr, "Error line %d: Undefined entry symbol '%s'\n", context->line_number, label);
                    context->error_found = TRUE;
                }
            }
        }

        // בהכרח פקודה או זבל כי את כל שאר המקרים כבר בדקנו
        else {
            cmd = get_command(first_word);
            // אם קיימת פקודה בשם הזה של המילה נאפס את המשתנים ששומרים על המקור והיעד.
            if (cmd) {
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));

                // כמו במעבר הראשון, בודקים לכמה אופרנדים מצפה הפקודה ומכניסים את האוגר למשתנה ששומר על המקור, אותו דבר עם היעד.
                if (cmd->expected_ops == 2) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        src[k++] = *line_ptr++;
                    }
                    src[k] = '\0';
                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') line_ptr++;
                    skip_whitespaces(&line_ptr);
                    k = 0;
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    // ציפינו ל2 אופרנדים וראינו רק אחד- שגיאה והדפסה למשתמש.
                    if (strlen(src) == 0 || strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand(s) for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    //חסרה כאן בדיקה של יותר מידי טקסט. בכללי יש יותר מידי קוד כפול-
                    // שווה לעבור על הקוד החדש של הגימניי ולעשות ניקיון והקלה על הקוד.


                    //נקרא במקום שתי השורות הללו שהן לא ברורות לפונקציה get_addressing_mode
                    //src_m = get_addressing_mode(src);
                    //dst_m = get_addressing_mode(dst);
                    // מחזירה את שיטת המיעון לכל אופרנד.
                    //בודק מה סוג שיטת המיעון עבור כל אופרנד. אם אין אופרנד חוקי או פקודה ללא אופרנד - -1.
                    src_m = (src[0] == '#') ? 0 : ((strlen(src) == 2 && src[0] == 'r' && src[1] >= '0' && src[1] <= '7') ? 3 : (src[0] == '%' ? 2 : 1));
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));

                    // שמירת שלוש מילים בטבלת הפקודות עבור הפקודה ושני האוגרים שלה
                    L = 3;
                }
                // אם מצפים רק לאופרנד אחד בפקודה- הכנסתו למשתנה ששומר על אוגר היעד ובדיקה שהוא נמצא
                else if (cmd->expected_ops == 1) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    // אם אין אוגר בפקודה שצריך להיות אופרנד אחד שגיאה והדפסה למשתמש
                    if (strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    // מוחקים את השורה הבאה
                    // ובמקומה מוסיפים: dst_m = get_addressing_mode(dst);
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));

                    // שמירה בטבלת הנתנוים מקום לשתי מילים עבור הפקודה הזו- קוד הפקודה ומילה לאופרנד.
                    L = 2;
                }

                skip_whitespaces(&line_ptr);
                //אם בפקודה שאמור להיות אופרנד אחד, אחרי שקראנו את המילה אין הורדת שורה או '/0'-
                // אז יש אקסטרה טקסט או יותר מידי אופרנדים עבור הפקודה הזו
                if (*line_ptr != '\0' && *line_ptr != '\n') {
                    fprintf(stderr, "Error line %d: Too many operands or extraneous text for command '%s'\n", context->line_number, first_word);
                    context->error_found = TRUE;
                    context->line_number++;
                    continue;
                }

                // כאן האסמבלר בודק אם אופרנד המקור הוא מסוג שדורש חיפוש בטבלת הסמלים:
                // בשיטה של מיעון ישיר ומיעון יחסי (1 ו2) אנחנו לא יודעים מה הכתובת בזמן הכתיבה וחייבים לשלוף אותה מהטבלה במעבר הראשון
                // בשיטה 2- האופרנד מתחיל ב-% ולכן נדלג על האחוז בחיפוש בטבלה ונחפש מהתו במקום אחד.
                // בשיטה אחד השם כבר נקי ללא משהו לפנייו, לכן נחפש אותו בטבלה כמו שהוא.
                if (src_m == 1 || src_m == 2) {
                    sym_name = (src_m == 2) ? src + 1 : src;
                    sym = get_symbol(context->symbol_head, sym_name);
                    // אם התווית בטבלת הסמלים
                    if (sym) {
                        // כתוב בחוברת הקרוס שעבור מיעון 2- לא ניתן להשתמש בתווית מקובץ חיצוני (extern).
                        if (sym->is_extern) {
                            if (src_m == 2) {
                                fprintf(stderr, "Error line %d: Cannot use relative addressing with external symbol '%s'\n", context->line_number, sym_name);
                                context->error_found = TRUE;
                                // אם זה שיטת מיעון 1, אנחנו עוד לא יודעים מה הערך של המילה, לכן נשים אפס (שמירת מקום), ונכתוב שזוהי פקודה מסוג E.
                            } else {
                                context->code_image[context->ic - INITIAL_IC + 1].value = 0;
                                context->code_image[context->ic - INITIAL_IC + 1].are = ARE_EXTERNAL;
                                // שמירת הנתנונים בחוליה של הרשימה של תוויות הextern עבור הדפסת הקובץ
                                // אנחנו מסתכלים על המקום ic+1 בטבלת הפקודות כי אנחנו צריכים את המילה השנייה (אופרנד המקור), המילה הראשונה שמורה למילת הפקודה.
                                add_ext(ext_list_head, sym->name, context->ic + 1);
                            }

                            // לא extern
                        } else {
                            if (src_m == 2) {
                                //אנחנו מסתכלים על המקום ic+1 בטבלת הפקודות כי אנחנו צריכים את המילה השנייה (אופרנד המקור), המילה הראשונה שמורה למילת הפקודה.
                                word_addr = context->ic + 1;
                                //בשיטת המיעון 2, המעבד לא אמור לקבל את הכתובת של התווית, אלא את המרחק אליה.
                                // הקוד מחסיר מהכתובת שבה התווית מוגדרת (sym->value) את הכתובת של המילה הנוכחית (word_addr).
                                // התוצאה היא המרחק (היסט) שעל המעבד "לקפוץ".
                                // אם ארוך מ12 ביטים- 0xFFF.
                                context->code_image[context->ic - INITIAL_IC + 1].value = (sym->value - word_addr) & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + 1].are = ARE_ABSOLUTE;
                            } else {
                                // עבור שיטת המיעון אחד, פשוט לוקחים את הכתובת של התווית כפי שנשמרה בטבלת הסמלים ושמים אותה במילה-
                                // השימוש ב-& 0xFFF מבטיח שהערך יישמר בטווח של 12 סיביות בלבד.
                                // שיטת מיעון זו מקבלת את סוג הפקודה R
                                context->code_image[context->ic - INITIAL_IC + 1].value = sym->value & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + 1].are = ARE_RELOCATABLE;
                            }
                        }
                        // התווית לא בטבלת הסמלים- הדפסת שגיאה למשתמש.
                    } else {
                        fprintf(stderr, "Error line %d: Undefined symbol '%s'\n", context->line_number, sym_name);
                        context->error_found = TRUE;
                    }
                }

                // אותו תהליך עבור אופרנד היעד.
                if (dst_m == 1 || dst_m == 2) {
                    sym_name = (dst_m == 2) ? dst + 1 : dst;
                    sym = get_symbol(context->symbol_head, sym_name);
                    offset = (src_m != -1) ? 2 : 1;

                    // אם התווית בטבלת הסמלים
                    if (sym) {
                        if (sym->is_extern) {
                            if (dst_m == 2) {
                                fprintf(stderr, "Error line %d: Cannot use relative addressing with external symbol '%s'\n", context->line_number, sym_name);
                                context->error_found = TRUE;
                            } else {
                                // המיקום של אופרנד היעד תלוי בשאלה: "האם היה אופרנד מקור לפניו?". כאן נכנס החישוב של ה-offset:
                                // אם היה אופרנד מקור, אז הoffset יהיה 2 כי יש לפני זה גם מילת פקודה וגם מילה עבור המקור.
                                // אם לא היה אופרנד מקור, הoffset יהיה 1 כי יש לפני רק את המילת פקודה.
                                context->code_image[context->ic - INITIAL_IC + offset].value = 0;
                                context->code_image[context->ic - INITIAL_IC + offset].are = ARE_EXTERNAL;
                                // שולחים את הכתובת המלאה (ic + offset) כי זה המידע שצריך להופיע בקובץ הפלט עבור הלינקר.
                                add_ext(ext_list_head, sym->name, context->ic + offset);
                            }
                        } else {
                            if (dst_m == 2) {
                                word_addr = context->ic + offset;
                                context->code_image[context->ic - INITIAL_IC + offset].value = (sym->value - word_addr) & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + offset].are = ARE_ABSOLUTE;
                            } else {
                                context->code_image[context->ic - INITIAL_IC + offset].value = sym->value & 0xFFF;
                                context->code_image[context->ic - INITIAL_IC + offset].are = ARE_RELOCATABLE;
                            }
                        }
                    } else {
                        // אם התווית לא ברשימת הסמלים, הדפסת שגיאה למשתמש.
                        fprintf(stderr, "Error line %d: Undefined symbol '%s'\n", context->line_number, sym_name);
                        context->error_found = TRUE;
                    }
                }

                // קידום מונה הפקודות והופסת כמות המילים עבור אותה השורה.
                context->ic += L;
            }
        }
        // מעבר לשורה הבאה.
        context->line_number++;
    }

    // אם נמצאה לפחות שגיאה אחת, נחזיר שקר (הפוך מאמת של error_found) ולהפך.
    return !context->error_found;
}
