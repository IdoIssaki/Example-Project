/*
 * קובץ: globals.h
 */
#ifndef GLOBALS_H
#define GLOBALS_H

#define MAX_LINE_LENGTH 80
#define MAX_LABEL_LENGTH 31
#define MEMORY_SIZE 4096
#define INITIAL_IC 100
#define NUM_COMMANDS 16

typedef enum {
    FALSE = 0,
    TRUE = 1
} boolean;

typedef enum {
    ARE_ABSOLUTE = 'A',
    ARE_RELOCATABLE = 'R',
    ARE_EXTERNAL = 'E'
} are_type;

typedef struct {
    unsigned int value; 
    are_type are;       
} machine_word;

typedef struct {
    char name[5];
    int opcode;
    int funct;
    int expected_ops;        /* מספר האופרנדים: 0, 1 או 2 */
    int valid_src_modes[4];  /* מערך של 4 תאים לחוקיות מקור */
    int valid_dest_modes[4]; /* מערך של 4 תאים לחוקיות יעד */
} command_entry;

typedef struct symbol_node {
    char name[MAX_LABEL_LENGTH + 1]; 
    int value;                       
    unsigned int is_code   : 1;
    unsigned int is_data   : 1;
    unsigned int is_entry  : 1;
    unsigned int is_extern : 1;
    struct symbol_node *next;        
} symbol_node;

typedef symbol_node *symbol_ptr;

typedef struct {
    int ic;                              
    int dc;                              
    int line_number;                     
    boolean error_found;                 
    machine_word code_image[MEMORY_SIZE]; 
    machine_word data_image[MEMORY_SIZE]; 
    symbol_ptr symbol_head;              
} AssemblerContext;

#endif
