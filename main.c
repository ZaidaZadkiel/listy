/*
gcc main.c listy5.c -o test
./test

(* (+ 1 2) 3) es codigo listy
son dos listas, una es
(* () 3 )

y la de a dentro es
(+ 1 2)


*/

#include "listy5.h"

int main(int atgc, char *argv[]){
printf("lisy6 test\n");


//print_list(eval("(* (/ 10 2) 3) "));

//#define SENV_TRACE  10   /* toggle tracing */
//#define SENV_PSTATE 20   /* print environment state */
//#define SENV_PPROG  30   /* print program tree */
//#define SENV_PMEMLS 35   /* print lists in memory */
//#define SENV_PNAMES 40   /* print defined names */
//#define SENV_RUN    80   /* run loaded program */
//#define SENV_ABORT  9999 /* immediatly abort */

/*
(+ (/ 10 2) 3)  \
(* (/ 10 2) 3)  \
 \
 (  ( + 10 20 30 5 40 80 ) ) \
*/

print_list(eval("(+ \
(% nombre (% parametro) (+ 1 parametro) ) \
(nombre (10))"));
}
