#ifndef __LISTY_OPS
#define __LISTY_OPS

/* code operators */
#define L_OPEN      '('  /* starts list */
#define L_CLOSE     ')'  /* ends list */
#define S_OPEN      '{'  /* starts a block */
#define S_CLOSE     '}'  /* ends a block */

/* SENV help text */
char *SENV_help =  
"10: trace toggle \t20: print state"           "\n"
"30: print program\t35: print lists in memory" "\n"
"40: print names  \t80: run loaded program."   "\n"
"9999: quit"                                   "\n";

/* SENV definitions */
#define SENV_TRACE  10   /* toggle tracing */
#define SENV_PSTATE 20   /* print environment state */
#define SENV_PPROG  30   /* print program tree */
#define SENV_PMEMLS 35   /* print lists in memory */
#define SENV_PNAMES 40   /* print defined names */
#define SENV_RUN    80   /* run loaded program */
#define SENV_ABORT  9999 /* immediatly abort */

/* env status flags */
#define ENV_TRON    0x1  /* bitflag for trace toggle */
#define ENV_LOADS   0x2  /* print load'd list toggle */
#define ENV_SOLVES  0x4  /* print solve'd list toggle */
#define ENV_REPLY   0x8  /* there is reply to be processed */
#define ENV_ERROR   0x8000 /* there has been an exception */

/* environment handling */
#define OP_DEF      '%'  /* defines a name (function or variable) */
#define OP_ACCENV   ':'  /* accesses the ENV of the parent list */
#define OP_SETV     '$'  /* sets a value XXX maybe. */
#define OP_SENV     '' /* send to env */

/* control structure */
#define OP_CMP      '?'  /* sets flag for comparision of lists */
#define OP_GT       '>'  /* evaluates it's list if comparision result is greater than */
#define OP_LT       '<'  /* ditto, but if less than */
#define OP_EQ       '='  /* ditto, but if both lists are equal */
#define OP_NEQ      '!'  /* ditto, but if the lists are not equal */

/* list management */
#define OP_QUO     '\'' /* returns a new list with quoted (not solved) params */
#define OP_HEAD    '.'  /* returns the rest of a list (. (1 2 3)) --> (1) */
#define OP_REST    '@'  /* returns the first element of a list (@ (1 2 3)) --> (2 3) */
#define OP_JOIN    '&'  /* returns a new list containing all the lists joined together (& (1 2)(3 4)) --> (1 2 3 4) */

/* still debatable
#define OP_REV     'æ'  // returns a reversed a list (æ (1 2)) --> (2 1) 
#define OP_REDEF   '!'  // redefines a previously created name (assigns a new list to an existing alias)
#define OP_LEN     '$'  // returns the size of a list (does it measures the included lists ?) ($ (1 2 3)) --> (3)
#define OP_AT      '#'  // returns a single element on the location indicated (# 2 (1 2 3)) --> (3) (0 based)
*/

/* arithmetic */
#define OP_SUB     '-'  /* substract second from first and creates new list with result */
#define OP_ADD     '+'  /* adds two elements into a new list */
#define OP_MUL     '*'  /* multiplies two elements into a new list */
#define OP_DIV     '/'  /* divides two elements into a new list */

/* stream handling */
#define OP_PUTS    '«'  /* put in stream */
#define OP_GETS    '»'  /* get from stream */

/* type definitions */
/* for marks[] bitmask */
#define TYPE_NULL    0
#define TYPE_NUMERIC 1 /* 0123456789 */
#define TYPE_STRING  2 /* {...} */ 
#define TYPE_LIST    4 /* () */ 
#define TYPE_NAME    8 /* a-zA-Z0-9 */
#define TYPE_SYMBOL 10 /* a name defined without defun */
#define TYPE_ENV    16
#define TYPE_OP     32

#endif
