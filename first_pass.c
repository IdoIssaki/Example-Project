#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "first_pass.h"
#include "parser.h"
#include "utils.h"
#include "tables.h"

boolean first_pass(FILE *am_file, AssemblerContext *context) {
    char line[MAX_LINE_LENGTH + 2];
    char first_word[MAX_LINE_LENGTH], label[MAX_LABEL_LENGTH];
    char *line_ptr;
    symbol_ptr curr;
    boolean expect_number;

    //מאפסים את מונה הפקודות ל100 כנדרש בחוברת קורס
    context->ic = INITIAL_IC;
    //dc- סופר את מספר ההנחיות הנתונים (data,string)
    context->dc = 0;
    //מתחילים מהשורה הראשונה
    context->line_number = 1;

    while (fgets(line, sizeof(line), am_file)) {
        //עדיף להעביר לאיפה שכל המשתנים
        boolean symbol_flag = FALSE;
        char num_str[MAX_LINE_LENGTH];
        const command_entry *cmd;
        char src[MAX_LINE_LENGTH];
        char dst[MAX_LINE_LENGTH];

        //שמירת שיטות המיעון ליעד ולמקור- כרגע -1 כי הם עוד לא נמצאו
        //L= אורך הפקודה במילים, לכל פקודה יש לפחות מילה אחת לכן מאותחל לאחד.
        int src_m = -1, dst_m = -1, L = 1;

        int val, offset, k;

        line_ptr = line;

        //אם זו שורת הערה-דלג לשורה הבאה
        if (is_empty_or_comment(line_ptr)) {
            context->line_number++;
            continue;
        }

        //הוצאת המילה הראשונה
        extract_word(&line_ptr, first_word);

        /* --- בדיקה אבסולוטית: האם התווית מתחילה באות? --- */
        if (strlen(first_word) > 0 && first_word[strlen(first_word) - 1] == ':') {
            if (!((first_word[0] >= 'A' && first_word[0] <= 'Z') || (first_word[0] >= 'a' && first_word[0] <= 'z'))) {
                fprintf(stderr, "Error line %d: Invalid label '%s' (Label MUST start with a letter)\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            //שם תקין לתווית- בלי סימנים שאסור וללא מילה שמורה.
            if (!is_label_definition(first_word)) {
                fprintf(stderr, "Error line %d: Invalid label syntax '%s'\n", context->line_number, first_word);
                context->error_found = TRUE;
                context->line_number++;
                continue;
            }
            //תווית תקינה- לכן מדליקים את הדגל
            symbol_flag = TRUE;
            //הוצאת ה':' והכנסת התווית למערך של התוווית.
            strncpy(label, first_word, strlen(first_word) - 1);
            label[strlen(first_word) - 1] = '\0';

            //הוצאת המילה הבאה
            extract_word(&line_ptr, first_word);
        }

        //כל החלק מפה עד הנקודה בהערה זה חלק שאמור להחליף הפונקציה בparser
        //parse_data_directive
        //אם המילה הראשונה היא data או string - סימן שאנחנו עוסקים בנתונים.
        if (strcmp(first_word, ".data") == 0 || strcmp(first_word, ".string") == 0) {
            //אם במילה הראשונה הייתה תווית חוקית ניכנס לפה כי הדגל דולק
            if (symbol_flag) {
                //שולחים את הסמל (label) עם הכתובת (גודל מונה הנתונים), וביט דולק עבור הדאתא
                //אם קיים כבר סמל עם אותו שם- שגיאה והודעה למשתמש
                if (!add_symbol(&context->symbol_head, label, context->dc, 0, 1, 0)) {
                    fprintf(stderr, "Error line %d: Label '%s' redefined.\n", context->line_number, label);
                    context->error_found = TRUE;
                }
            }
            if (strcmp(first_word, ".data") == 0) {
                //  במקום כל הבלוז הזה נקרא פה לפונקציה- parse_data_directive(&line_ptr, context)
                //משתנה ששומר על התחביר של מספר, מספר,..... ,אם הוא אמת, צריך להיות מספר עכשיו לפי התחביר.
                expect_number = TRUE;
                while (*line_ptr != '\0' && *line_ptr != '\n') {
                    skip_whitespaces(&line_ptr);

                    //צריך את התנאי הזה שוב כדי לוודא שאחרי שהורדנו את הרווחים לא סיימנו את השורה/ הקובץ.
                    // למשל עם מישהו רשם data ואז מספר ואז מלא רווחים וירד שורה.
                    if (*line_ptr == '\0' || *line_ptr == '\n') break;
                    
                    if (*line_ptr == ',') {
                        //אם הדגל דלוק- ציפנו למספר (אחרת נמשיך למילה הבאה והפעם נצפה למספר אחרי הפסיק).
                        // לכן שגיאה והדפסה למשתמש.
                        if (expect_number) {
                            fprintf(stderr, "Error line %d: Unexpected comma in .data\n", context->line_number);
                            context->error_found = TRUE;
                            break;
                        }
                        line_ptr++;
                        expect_number = TRUE;
                        continue;
                    }

                    //אם המשתמש לא ציפה למספר ולא שם ',' אחרי המספר. שגיאה והדפסה למשתמש
                    if (!expect_number) {
                        fprintf(stderr, "Error line %d: Expected comma between numbers\n", context->line_number);
                        context->error_found = TRUE;
                        break;
                    }

                    // הוצאת המילה הבאה למשתנה num_str
                    extract_word(&line_ptr, num_str);

                    if (num_str[0] != '\0') {
                        //מספר לא חוקי- שגיאה והדפסה למשתמש
                        if (!is_valid_number(num_str)) {
                            fprintf(stderr, "Error line %d: Invalid number '%s' in .data\n", context->line_number, num_str);
                            context->error_found = TRUE;
                            break;
                        }

                        //הכנסת המספר כערכו ולא כמחרוזת בתוך טבלת הנתונים במקום הנכון לו לפי כמות הנתונים (המונה dc)
                        // אם המשתמש הכניס מספר שגודלו הוא יותר מ12 ביטים, ניקח את המספר 0xFFF שהוא 1......1 (12 פעמים) בהקסה דצימלי,
                        // כי מילה לא יכולה להיות יותר מ12 ביטים
                        // כל מספר בdata הוא מילה, וגם כל מחרוזת בstring
                        context->data_image[context->dc].value = atoi(num_str) & 0xFFF;
                        //מוכנס גם כמילה באותו המקום של מונה הנתונים גם סוג הפקודה (ARE)
                        // עבור נתנונים תמיד ניקח אבסולוט (A)
                        context->data_image[context->dc].are = 'A';
                        //קידום מונה הנתונים
                        context->dc++;
                        // לאחר הכנסת המספר, אנחנו לא מצפים יותר למספר אלא או לסיום השורה או ל-',' .
                        expect_number = FALSE;
                    }
                    //עד לפה .
                }
            } else { /* טיפול ב- .string */
                //גם פה במקום הבלוק הזה נקרא במקום לפונקציה parse_string_directive(&line_ptr, context)
                skip_whitespaces(&line_ptr);

                /* 1. בדיקה שיש מרכאה פותחת */
                // תו המרכאות- '\"'
                if (*line_ptr != '\"') {
                    fprintf(stderr, "Error line %d: Missing opening quote for .string directive\n", context->line_number);
                    context->error_found = TRUE;
                } else {
                    line_ptr++; /* מדלגים על המרכאה הפותחת */

                    /* מכניסים את התווים למערך */
                    while (*line_ptr != '\"' && *line_ptr != '\0' && *line_ptr != '\n') {
                        // אותו דבר כמו עם data מכניסים את המחרוזת במקום המתאים בטבלת הנתנים לפי מונה הנתונים
                        // (אם המחרוזת גדולה מ12 סיביות, ניקח את 0xFF)
                        // תתייחס למספר הזה כאל מספר חיובי שלם - (unsigned int)
                        // נשים לב שמילה של מחרוזת זה מילה לכל תו- כלומר לכל אות.
                        context->data_image[context->dc].value = (unsigned int)*line_ptr & 0xFFF;
                        context->data_image[context->dc].are = 'A';
                        context->dc++;
                        line_ptr++;
                    }

                    /* 2. הגענו לסוף - בודקים אם עצרנו בגלל מרכאה סוגרת או בגלל שנגמרה השורה */
                    if (*line_ptr != '\"') {
                        //אין מרכאות בסוף המחרוזת- שגיאה והדפסה למשתמש.
                        fprintf(stderr, "Error line %d: Missing closing quote for .string directive\n", context->line_number);
                        context->error_found = TRUE;
                    } else {
                        /* סיימנו בהצלחה! סוגרים את המחרוזת עם תו ה-NULL (אפס) */
                        context->data_image[context->dc].value = 0;
                        context->data_image[context->dc].are = 'A';
                        context->dc++;

                        line_ptr++; /* מדלגים על המרכאה הסוגרת */

                        /* 3. בדיקה שאין טקסט מיותר אחרי המחרוזת */
                        skip_whitespaces(&line_ptr);
                        if (*line_ptr != '\0' && *line_ptr != '\n') {
                            // שגיאה- טקסט אחרי המחרוזת, והדפסה למשתמש
                            fprintf(stderr, "Error line %d: Extraneous text after string in .string directive\n", context->line_number);
                            context->error_found = TRUE;
                        }
                        //עד לפה .
                    }
                }
            }
        }
        // המילה הראשונה היא extern
        else if (strcmp(first_word, ".extern") == 0) {
            extract_word(&line_ptr, label);
            if (label[0] != '\0') {
                /* בדיקה שהאקסטרן מתחיל באות */
                if (!((label[0] >= 'A' && label[0] <= 'Z') || (label[0] >= 'a' && label[0] <= 'z'))) {
                    fprintf(stderr, "Error line %d: Invalid extern label '%s' (Must start with a letter)\n", context->line_number, label);
                    context->error_found = TRUE;
                } else {
                    /* --- הנה התיקון שלנו: בודקים אם הסמל כבר קיים בבית --- */
                    // ממולץ להכריז בהתחלה של הקוד על המשתנה ולא כאן
                    // פה לא מדפיסים ישר שגיאה אם נמצא הסמל כבר כי לפי הקומפילר זו לא טעות להגדיר פעמיים תווית כextern
                    symbol_ptr existing = get_symbol(context->symbol_head, label);

                    if (existing != NULL) {
                        /* מצאנו אותו בטבלה! האם הוא הוגדר פה כמקומי? */
                        //אם הוא לא extern אלא מוגדר כ-entery זה בעייתי להגדיר תווית חדשה כextern.
                        // שגיאה והדפסה למשתמש.
                        if (!existing->is_extern) {
                            fprintf(stderr, "Error line %d: Symbol '%s' is already defined locally. Cannot declare as .extern\n", context->line_number, label);
                            context->error_found = TRUE;
                        }
                    } else {
                        /* הוא לא קיים עדיין, אז מותר להגדיר אותו כחיצוני בבטחה */
                        add_symbol(&context->symbol_head, label, 0, 0, 0, 1);
                    }
                }
            }
        }
        // חייב את זה כדי שלא יפול בטעות בelse וידפיס שגיאה.
        // אפשר גם להוסיף פה לטבלת הסמלים- לפני המעבר השני, אבל לא חובה.
        else if (strcmp(first_word, ".entry") == 0) {
            /* המעבר השני מטפל בזה */
        }
        else {
            //לא פקודה חוקית.
            cmd = get_command(first_word);
            if (!cmd) {
                fprintf(stderr, "Error line %d: Unknown command '%s'.\n", context->line_number, first_word);
                context->error_found = TRUE;
            } else {
                //אתחול אפסים באוגר היעד ואוגר המקור.
                memset(src, 0, sizeof(src));
                memset(dst, 0, sizeof(dst));

                //אם זה דולק, נתקלנו בתווית, לכן נוסיף לטבלת הסמלים, אם לא קיימת.
                // LOOP: mov r1, r2- למשל זו תווית שמונה הפקודות יספור.
                if (symbol_flag) {
                    /* מנסים להוסיף, ואם הפונקציה מחזירה שקר - סימן שהתווית כבר קיימת! */
                    // מכניסים עם תווית code דלוקה.
                    if (!add_symbol(&context->symbol_head, label, context->ic, 1, 0, 0)) {
                        fprintf(stderr, "Error line %d: Label '%s' is already defined.\n", context->line_number, label);
                        context->error_found = TRUE;
                    }
                }
                // במקום כל הבלוק הזה נקרא לפונקציה-
                // במקומו מוסיפים קריאה לפונקציה:
                // if (cmd->expected_ops > 0) parse_command_operands(&line_ptr, src, dst, cmd->expected_ops, context->line_number);

                // התנאי הזה הוא טיפול עבור פקודות שדורשות 2 אופרנדים
                if (cmd->expected_ops == 2) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    // כל עוד לא הגענו לסיום המחרוזת או פסיק או ' ' או '\t' או '\n'
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        // העתקת התו במיקום k מהאופרנד ל-src
                        src[k++] = *line_ptr++;
                    }
                    src[k] = '\0';

                    skip_whitespaces(&line_ptr);
                    if (*line_ptr == ',') line_ptr++;

                    //אותו דבר עבור האופרנד השני
                    skip_whitespaces(&line_ptr);
                    k = 0;
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    // שגיאה והדפסה למשתמש שחסר אופרנד.
                    if (strlen(src) == 0 || strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand(s) for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    //עד לפה מוחקים ושמים במקום את הקריאה לפונקציה והתנאי

                    //נקרא במקום שתי השורות הללו שהן לא ברורות לפונקציה get_addressing_mode
                    //src_m = get_addressing_mode(src);
                    //dst_m = get_addressing_mode(dst);
                    //בודק מה סוג שיטת המיעון עבור כל אופרנד. אם אין אופרנד חוקי- -1.
                    src_m = (src[0] == '#') ? 0 : ((strlen(src) == 2 && src[0] == 'r' && src[1] >= '0' && src[1] <= '7') ? 3 : (src[0] == '%' ? 2 : 1));
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));

                    /* בדיקת שיטות מיעון מותרות */
                    //אם קיבלנו -1 למשל , השיטת מיעון לא חוקית. שגיאה והדפסה למשתמש.
                    // יכול להיות גם שקיבלנו שיטת מיעון לא חוקית עבור פקודה מסוימת.
                    //אפשר לצמצם לתנאי אחד עם או בינהם
                    if (!is_valid_addr_mode(cmd->valid_src_modes, src_m)) {
                        fprintf(stderr, "Error line %d: Invalid source addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    if (!is_valid_addr_mode(cmd->valid_dest_modes, dst_m)) {
                        fprintf(stderr, "Error line %d: Invalid dest addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    
                    /* בדיקת ערך מיידי תקין */
                    //קוד שלא מגיעים אליו- קוד מת! צריך לפתור
                    //הsrc פלוס אחד זה כדי לדלג על הסולמית.
                    if (src_m == 0 && !is_valid_number(src + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, src);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }
                    if (dst_m == 0 && !is_valid_number(dst + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, dst);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    //צריך 3 מילים עבור השורה בטבלת הפקודות
                    L = 3;
                }

                //אותו דבר עבור פקודות שמצפות לאופרנד אחד.
                else if (cmd->expected_ops == 1) {
                    k = 0;
                    skip_whitespaces(&line_ptr);
                    while (*line_ptr && *line_ptr != ',' && *line_ptr != ' ' && *line_ptr != '\t' && *line_ptr != '\n') {
                        dst[k++] = *line_ptr++;
                    }
                    dst[k] = '\0';

                    if (strlen(dst) == 0) {
                        fprintf(stderr, "Error line %d: Missing operand for command '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    // מוחקים את השורה הבאה (228)
                    // ובמקומה מוסיפים: dst_m = get_addressing_mode(dst);
                    dst_m = (dst[0] == '#') ? 0 : ((strlen(dst) == 2 && dst[0] == 'r' && dst[1] >= '0' && dst[1] <= '7') ? 3 : (dst[0] == '%' ? 2 : 1));
                    
                    /* בדיקת שיטת מיעון מותרת */
                    if (!is_valid_addr_mode(cmd->valid_dest_modes, dst_m)) {
                        fprintf(stderr, "Error line %d: Invalid dest addressing mode for '%s'\n", context->line_number, first_word);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    //קוד לא תקין-קוד מת. לתקן!
                    /* בדיקת ערך מיידי תקין */
                    //הפלוס אחד זה כדי לדלג על הסולמית.
                    if (dst_m == 0 && !is_valid_number(dst + 1)) {
                        fprintf(stderr, "Error line %d: Invalid immediate value '%s'\n", context->line_number, dst);
                        context->error_found = TRUE;
                        context->line_number++;
                        continue;
                    }

                    //שמירת שתי מילים בטבלת הפקודות עבור השורה הזו.
                    L = 2;
                }

                /* --- בדיקת טקסט מיותר לכל הפקודות (כולל stop שמקבלת 0) --- */
                skip_whitespaces(&line_ptr);
                if (*line_ptr != '\0' && *line_ptr != '\n') {
                    fprintf(stderr, "Error line %d: Too many operands or extraneous text for command '%s'\n", context->line_number, first_word);
                    context->error_found = TRUE;
                    context->line_number++;
                    continue;
                }

                //מחסרים מic 100 כדי לשבץ כל מילה במקום המתאים לה בטבלת הפקודות.
                context->code_image[context->ic - INITIAL_IC].value =
                        //מגדירים עבור המילת פקודה מיקומי ביטים
                        (cmd->opcode << 8) |
                        (cmd->funct << 4) |
                        /* * טיפול במקור: אם src_m הוא 1- (אין אופרנד), אחרת קח את הערך של src_m, נשתמש ב-0 כדי לא ללכלך את הביטים.
                             * לאחר מכן מזיזים 2 ביטים שמאלה כדי להגיע למיקום הנכון (ביטים 3-2).
                        */
                        ((src_m != -1 ? src_m : 0) << 2) |
                         /* * טיפול ביעד: אם dst_m הוא 1- (אין אופרנד), נשתמש ב-0', אחרת קח את הערך של dst_m.
                         * אין צורך בהזזה (Shift) כי אלו הביטים הכי ימניים (1-0).
                          */
                        (dst_m != -1 ? dst_m : 0);
                        //כאן מגדירים את השורת פקודה כA כי עוד אין קפיצות זה קורה בsecond pass
                context->code_image[context->ic - INITIAL_IC].are = ARE_ABSOLUTE;

                if (src_m != -1) {
                    //הקוד בודק אם מדובר במספר (שיטה 0) או באוגר (שיטה 3). אלו ערכים שאנחנו יודעים מיד, לכן אפשר לקודד אותם כבר עכשיו.
                    if (src_m == 0 || src_m == 3) {
                        //עבור השיטת מיעון אפס ניקח את המספר אחרי הסולמית
                        /* * 1. src[1] - '0': המרת תו ה-ASCII של מספר האוגר לערך מספרי (למשל '5' הופך ל-5).
                        * 2. 1 << X: הדלקת ביט מספר X במילה כדי לייצג את האוגר שנבחר.
                        */

                        //כל התנאי קוד מת- לבדוק איך לתקן!
                        val = (src_m == 0) ? atoi(src + 1) : (1 << (src[1] - '0'));

                        //במיקום ic + 1: נמצא תמיד המידע של אופרנד המקור (src).
                        //במיקום ic + 2 (או ic + 1 אם אין מקור): נמצא המידע של אופרנד היעד (dst).
                        context->code_image[context->ic - INITIAL_IC + 1].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + 1].are = ARE_ABSOLUTE;
                    } else {
                        //לפה נכנסים אם אנחנו בשיטת מיעון 1, אנחנו לא יודעים את סוג הפעולה והכתובת במקרה זה ולכן 0.
                        context->code_image[context->ic - INITIAL_IC + 1].value = 0;
                        context->code_image[context->ic - INITIAL_IC + 1].are = '\0';
                    }
                }
                if (dst_m != -1) {
                    // קביעת מיקום אופרנד היעד: מרחק 2 מילים אם קיים אופרנד מקור (שתופס את מילה 1), אחרת מרחק מילה אחת בלבד.
                    offset = (src_m != -1) ? 2 : 1;
                    //החלק הזה זהה כמו שעשינו ל- src_m
                    if (dst_m == 0 || dst_m == 3) {
                        //כל התנאי קוד מת- לתקן!
                        val = (dst_m == 0) ? atoi(dst + 1) : (1 << (dst[1] - '0'));
                        context->code_image[context->ic - INITIAL_IC + offset].value = val & 0xFFF;
                        context->code_image[context->ic - INITIAL_IC + offset].are = ARE_ABSOLUTE;
                    } else {
                        context->code_image[context->ic - INITIAL_IC + offset].value = 0;
                        context->code_image[context->ic - INITIAL_IC + offset].are = '\0';
                    }
                }

                /* עדכון מונה הפקודות (IC) בערך L - מספר המילים שהפקודה הנוכחית תפסה בזיכרון.
                פעולה זו מבטיחה שהפקודה/תווית הבאה תקבל את הכתובת הפנויה הנכונה. */
                context->ic += L;
                context->ic += L;
            }
        }
        context->line_number++;
    }

    //מתחיל לעבור על רשימת הסמלים (התוויות) ששמרנו
    curr = context->symbol_head;
    while (curr) {
        //אם התווית שייכת לנתונים, נוסיף לה גם את הערך הסופי של ic כדי שנדע איזה כתובת אנחנו
        // כך טבלת הנתונים נדחפת לסוף הקוד.
        if (curr->is_data) curr->value += context->ic;
        curr = curr->next;
    }

    // אם נמצאה לפחות שגיאה אחת, נחזיר שקר (הפוך מאמת של error_found) ולהפך.
    return !context->error_found;
}
