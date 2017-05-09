#ifndef __LISTY5
#define _GNU_SOURCE /* to have strndup */
#define __LISTY5

#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* general definitions */
#define true    -1
#define false    0
#define MAX_LEN    1024  

typedef struct env  env;
typedef struct name name;
typedef struct list list;
typedef int    list_type;

char  *text;
/* reply buffer */
char *buffer[1024];

struct list {
  int   type;
  union {
    int    d;
    float  f;
    char  *s;
    list  *l;
    env   *e;
    name  *n;
  } data;
  list *n; /* next list, if == NULL, this is the last element */
};

/* holds a defined name string and list value */
struct name {
  env   *e;    // function internal environment (for local names), must be destroyed on exit.
  char  *name; // the string representing the defun'd symbol
  //char **args; /* maybe could be changed to list **args, which would help type strength */
  list  *args; // the list pointing to the named args, for local buffer
  list  *body; // list pointed to by the name symbol
};

struct env{
  char *code; /*                                                */
  int   len;  /* this three elements are not needed for evaling */
  int   ptr;  /*                                                */

  list *program; /* holds the program in the listy */
  
  struct {
    name **ptr;  /* malloc'd area for array of strings */
    int    len;  /* length of malloc'd area */
    int    used; /* how many names are stored*/
  } names;
  
  env *parent;
  int  flags;
  int  running;
}; /* struct env */

/* misc internal */
int    test                 (char*);
int    iswspc               (char);
int    isnum                (char);
int    get_numeric          (char*);
char*  get_string           (char*);
int    set_conn_fd          (int);
char*  find_printable_type  (list_type);

/* marks */
int    get_next_mark        (int);
int    set_mark             (int, int);

/* environment */
void   print_env            (env*);
env   *create_env           (env *);
int    init_env             (env*, char*);

/* names */
void   print_names          (env*);
void   print_name           (name*);
int    add_name             (name*, env*);
name  *find_name            (char*, env*);
name  *create_name          (char*);

/* text code */
list  *eval                 (char*);
env   *load                 (char*);

/* lists */
list  *create_list          (list*);
list  *create_list_numeric  (int);
list  *copy_list            (list*, list*);
void   free_list            (list*);
list  *load_we              (env*);
list  *solve_list_we        (list*, env*);
list  *solve_list           (list*);
list  *solve_op             (list*, env*);
list  *solve_name           (list*, env*);
list  *set_list_numeric     (list*, int);
int    count_elements       (list*);
int    count_all_lists      ();
int    find_list            (list *l);

/* printing text */
void   print_list           (list*);
void   print_list_wd        (list*, int);
void   print_type           (list*);
void   print_value          (list*);
void   print_all_lists      ();
int    render_list          (list*, char*, int);

#endif
