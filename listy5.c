/*
sep 2 2016: TODO implement reply2 as a readable buffer to be used in the main.c and send it thru socket to client
sep 11    : TODO implement a "append to list" function so as to avoid checking and setting everywhere list->n when loading from text

TODO: implement floatng point numbers. 
a possible solution is to create a list type "fraccionary" (fraccionary-symbol exponent fraccionary)
so that when you need an integer you can call exponent directly, and if you do any operation with the fraccionary part you can just assume 0 for whole numbers 
(i.e. not fractionary)

( + 23.54 12.23 ) parses to ( + (fractionary 23 54) (fractionary 12 23) )
so the operation is
(fractionary (+ 23 12) ( + 54 23 ))

*/

/*
 * % defun
 * ' quote
 * # get Nth element
 * @ get Nth-rest list
 * & join
 * = compare / equal
 * < compare / less than
 * > compare / greater than
 * | logic or ?
 * & logic and ?
 * « get from stream
 * » put to stream
 */

#define DEBUG_LISTY

#include "listy5.h"
#include "listy5_ops.h"

#include <sys/socket.h>

list *lists[MAX_LEN]; /* holds all used lists */
name *names[MAX_LEN]; /* holds all used names */
env  *envs [MAX_LEN]; /* holds all used environments */
int  marks [MAX_LEN]; /* marks used elements */

static list list_empty = { 4, {0}, NULL};
list stack [MAX_LEN];

long solve_call  = 0;
int text_len     = 1000;

/* reply read = 1; no reply = 0 */
int reply_ready  = 0;
int buffer_ptr   = 0;
int conn_fd      = 0;

int printable_src = 0;
char *find_printable_type(list_type t){
  switch(t){
    case TYPE_NULL:    return "()      ";
    case TYPE_NUMERIC: return "numeric "; 
    case TYPE_STRING:  return "{}      "; 
    case TYPE_LIST:    return "(list)  "; 
    case TYPE_NAME:    return "name    "; 
    case TYPE_SYMBOL:  return "symbol  ";
    case TYPE_ENV:     return "environment";
    case TYPE_OP:      return "operator";
  } //swtich(t)
  return "type error";
}


int set_conn_fd(int fd){
  printf("listy: conn_fd is %d\n", conn_fd);
  conn_fd = fd;
  send(conn_fd, "ready\r\n", 8, 0);
  return conn_fd;
}

char *reply_socket(int conn_fd, char *str){
  printf("socket: %d, str: %s\n", conn_fd, str);
  return str;
}

/**/
char *reply2(const  char *fmt, ...){
//char *reply2(const  char *text){
  if(!text){
    text = malloc(text_len);
  }

  int len;

  if(fmt && *fmt){
    va_list args;
    va_start(args, fmt);
    len = vsnprintf(text, text_len, fmt, args);
    va_end(args);
  }

  //send(conn_fd, text, len, 0);
//  strcpy((char*)&buffer[buffer_ptr], text);
//  buffer_ptr += len;
//  reply_ready = 1;
  printf("%s", text);
  return text;
}
/**/


#ifdef L_strndup
char *strndup(const char *str, size_t len)
{
  register size_t n;
  register char *dst;

  n = strlen(str);
  if (len < n) n = len;
  dst = (char *) malloc(n+1);
  if (dst) {
    memcpy(dst, str, n);
    dst[n] = '\0';
  }
  return dst;
}
#endif


/* creae empty execution environment and solves list */
list *solve_list(list *l){
  env *e = create_env(NULL);

  #ifdef DEBUG_LISTY
  e->flags |= ENV_TRON;
  #endif // DEBUG_LISTY

  return solve_list_we(l, e);
}

/* 
convert a list into a text representation 
returns a char* malloc'd which must be freed after use
*/
int render_list (list *l, char *text, int text_len){
  
  list *head = l;
  int used = 0;

  if(l == NULL) {
    used += sprintf(&text[used], "NULLPTR");
    return used;
  }
// struct list {
//   int   type;
//   union {
//     int    d;
//     float  f;
//     char  *s;
//     list  *l;
//     env   *e;
//     name  *n;
//   } data;
//   list *n; /* next list, if == NULL, this is the last element */
// }; 

  int do_break = 0;
  //printf("render_list \n");
  while(l != NULL){
    //used += sprintf(&text[used], "x%d ", used);
    
    switch(l->type){
      case TYPE_NULL: printf("null %d\n", 0); break;
      case TYPE_NUMERIC: {
        //printf("N %d %d \n", l->data.d, (int)l->n);
        used += sprintf(&text[used], "%d%s", l->data.d, (l->n ? " " : "") );
        break;
      } // TYPE_NUMERIC
      case TYPE_STRING: 
        //printf("S %d \n", l->data.d);
        used += sprintf(&text[used], "S{%s}", l->data.s);
        break;
      case TYPE_NAME: 
        //printf("NA %d \n", l->data.d);
        if( (l->data.n != NULL) ) {
          if( (l->data.n->name != NULL) )used += sprintf(&text[used], "%s", l->data.n->name);
        } else {
          reply2("render_list error: name is null\n");
        } //if l->data or l->data->name 
        break;
      case TYPE_SYMBOL: used += sprintf(&text[used], "%s", l->data.s); break;
      case TYPE_ENV:
        //printf("E %d \n", l->data.d);
        used += sprintf(&text[used], "E %d", 1); 
        break;
      case TYPE_OP:
        //printf("O %c \n", l->data.d);
        used += sprintf(&text[used], "%c ", l->data.d);
        break;
      case TYPE_LIST: {
        //printf("L %d \n", l->data.d);
        used += sprintf(&text[used], "(");
        
        // list *l, char *text, int text_len
        used += render_list(l->data.l, &text[used], text_len-used);
        used += sprintf(&text[used], ")");
//        if(l->data.l != NULL) {
//          l = l->data.l;
//          continue;
//        }
        break;
      } // TYPE_LIST
      default:
        used += sprintf(&text[used], "xxx");
        break;
    }// switch
    
    l = l->n;
    if(l == NULL){
      //printf("end \n");
    }else{    
      //printf("next \n");
    }
  }// while
  
  //printf("render_list %s\n", text);
  return used;
} // render_list(list *l, text, len)

/* solve list with environment */
list *solve_list_we(list *l, env *e){
  /* called to solve a null list, do nothing whatsoever */
  if(l == NULL) {
    printf("solving null list!\n");
    return NULL; 
  }
  
  list *res  = NULL; //create_list(NULL);
  unsigned long  call = solve_call++;
  
  char x[200];
  render_list(l, x, 200);
  reply2("step %04x enter %s (%s)\n", call, find_printable_type(l->type), x);

  if(e->flags & ENV_ERROR){
    printf("error!\n");
  }

  //if(l->n == NULL ) printf("solve_list_we warning: l->n is NULL\n");

// solve the list
  switch(l->type){
    /* solve list pointed-to by this list */
    case TYPE_LIST:
      res = solve_list_we(l->data.l, e);
      //reply2("solve_list_we result res %d\n", res);
      l->data.l = res->data.l;
      l->type   = res->type;
      break;
    case TYPE_OP:{
      res = solve_op(l, e);
      break;
    }
    case TYPE_SYMBOL:
    case TYPE_NAME:
      res = solve_name(l, e);
      break;
    case TYPE_NUMERIC:
      res = l;
      break;
    default:
      print_type(l);
      reply2(" %d is not recognized type\n", find_printable_type(l->type));
      break;
  } /* switch(l->type) */

  if( //l->type != TYPE_NUMERIC &&
#ifdef DEBUG_LISTY  
    true
#else
    e->flags & ENV_TRON
#endif
  ){
    //reply2("flag\n");
    char input[20];
    render_list(l, input, 20);
    char answer[20];
    render_list(res , answer, 20);
    reply2(
      "step %04x exit  %s (%s) => (%s)\n", 
      call,
      (res != NULL ? find_printable_type(res->type) : "null"),
      input,
      answer
    );  
  }

  e->flags |= ENV_REPLY;
  return res;
} /* solve_list_we(env) */

list *last_list(list *l){
  if(l == NULL) return NULL;
  while(l->n != NULL) l = l->n;
  return l;
}

list *set_name_data(name *n, list* args, list *body, env *e){
  reply2("this should substitute the correponding env->name\n");
  n->name = "test";
  n->e    = e;
  n->args = args;
  n->body = body;
  return ;
}
//do_name(e->names.ptr[i], l->n, e);
list *do_name(name *n, list *l, env *e){
  //print_name(n);
  
  if(n->args != NULL){
    char x[20];
    render_list(l, x, 20);
    reply2("do_name function with l = %s\n", x);
    env *scope = e;//create_env(e); //create scope for this function call
    
    list *last = last_list(l);
    print_list(last);
    
    reply2("solving args\n");
    list *args = solve_list_we(n->args, scope);
    if(args->type == TYPE_NAME) set_name_data(args->data.n, NULL, l, e);
    
    print_name(args->data.n);
    
    reply2("solving body\n");
    list *body = solve_list_we(n->body, scope);
    
    print_list(args);
    print_list(body);
    reply2("end do_name\n");
    return body;
  }else {
    char x[100];
    render_list(l, x, 100);
    reply2("do_name symbol substitution for name '%s' with l=%s\n", n->name, find_printable_type(n->body->type));
    
    int i = 0;
    while(strcmp(n->name, e->names.ptr[i]->name) != 0) {
      reply2("e->names.ptr[%d] == %s\n", i, e->names.ptr[i]->name);
      i++;
    }
    render_list(e->names.ptr[i]->body, x, 20);
    reply2("found %s\n", x);
    list *res = create_list(solve_list_we(e->names.ptr[i]->body, e));
    res->n = 0;
    return res;
  }
  
  return NULL;
}


list *solve_name(list *l, env *e){
  if(l == NULL){
    reply2("error: list is NULL\n");
    return NULL;
  }

  list *res;

  if(l->type == TYPE_SYMBOL){
    /* search for name called l->data.s */
    /* return new list(name) */
    reply2("solve_name: searching for symbol '%s'\n", l->data.s);
    
    char *target, *source;
    target = l->data.s;
    int i;
    for(i = 0; i != e->names.len; i++){
      //reply2("len %d %s\n", , e->names.ptr[0]->name);
      source = e->names.ptr[i]->name;
      if(target[0] == source[0]) //check first character
        if(strcmp(target, source) == 0) break;
    }
    
    
    print_list(l);
    reply2("what is next ?");
    
    if(l->n == NULL){ 
      //single symbol 
      reply2(" null\n");
      res = do_name(e->names.ptr[i], l, e);
    } else { 
      reply2(" a list\n");
      print_list(l->n);
      res = solve_list_we(l->n, e);
    }
  } else if(l->type == TYPE_NAME){
    reply2("solve_name: found name\n");
    /* evaluate list pointed by l->data.n->l */
    /* return new list(eval(l->data.n->l)); */
    print_type(l); reply2(" ");
    print_value(l); reply2("\n");
  } else {
    reply2("solve_name: list is not NAME or SYMBOL\n");
  }

  return res;
}

name *find_name(char *target, env *e){
  if(target == NULL){
    reply2("error: find name target is NULL\n");
    return NULL;
  }

  name **n = e->names.ptr;
  int i = 0 ;
  char *str; /* string in e->names.ptr[x] */
  while(i != e->names.used){
    str = n[i]->name;
    if(str[0] == target[0]){
      /* reply2("match\n"); */
      if(strcmp(str, target) == 0){
        /* reply2("perfect match\n"); */
        name *res = n[i]; /* create a copy of the name's list */
        return res;
      }
    }
    i++;
  }
  return NULL;
}

int add_name(name *n, env *e){
  if(n == NULL){
    reply2("error: attempted to add a null name\n");
    return false;
  }
  
  if(e->names.ptr == NULL){
    e->names.len = 16;
    e->names.ptr = malloc(sizeof(name*) * e->names.len);
    if(e->names.ptr == NULL){
      reply2("error: can't malloc %d names\n", e->names.len);
    }
  }

  if((e->names.used) == e->names.len){
    /* realloc */
    reply2("names.used equals names.len\n");
    e->names.len *= 2;
    e->names.ptr = realloc(e->names.ptr, sizeof(name*) * e->names.len);
  }

  e->names.ptr[e->names.used] = n;
  if(false){
    reply2("name n:\n\tname:\t%x\n\targs:\t%x\n\tl:\t%x\n",
        (unsigned long)n->name,
        (unsigned long)n->args,
        (unsigned long)n->body
      );

    name *p = e->names.ptr[e->names.used];
    reply2("names.ptr[%d]:\n\tname:\t%x\n\targs:\t%x\n\tl:\t%x\n",
        e->names.used,
        (unsigned long)p->name,
        (unsigned long)p->args,
        (unsigned long)p->body
      );
  }/* if debug */
  e->names.used++;
  return true;
}

list *solve_param(list *l, list_type type, env *e){
  list *res = solve_list_we(l, e);
  /* FIXME: should print the desired type */
  if(res->type != type){
    reply2("warning: argument is not numeric");
  }
  return res;
}

list *do_add(list *l, env *e){
//  printf("do_add\n");
  if(l == NULL) {
    reply2("error: null list\n"); 
    return NULL;
  }
  if(l->n == NULL){
    reply2("warning: n is null\n");
  }
  
  list *h = create_list(l);
  h->n = NULL; //detach head
  h = solve_list_we(h, e);
  
  list *r = l->n;
  if(r && r->n != NULL){
    r = do_add(r, e);
  }else{
    r = solve_list_we(r, e);
  }
  
  if(h->type == TYPE_NUMERIC && r->type == TYPE_NUMERIC){
    h->data.d += r->data.d;  
  } else {
    reply2("do_add error: h type is %s and r type is %s, both should be numeric\n", find_printable_type(h->type), find_printable_type(r->type));
  }
  
  
  
#ifdef DEBUG_LISTY
  char res  [100];
  char input[100];
  render_list(h, res,   100);
  render_list(l, input, 100);
  printf("(+ %s) = (%s)\n", input, res);
#endif

  return h;
}

list *do_sub(list *l, env *e){
//  printf("do_sub\n");
  if(l == NULL) {
    reply2("error: null list\n"); 
    return NULL;
  }
  if(l->n == NULL){
    reply2("warning: n is null\n");
  }
  
  list *h = create_list(l);
  h->n = NULL; //detach head
  h = solve_list_we(h, e);
  
  list *r = l->n;
  if(r && r->n != NULL){
    r = do_sub(r, e);
  }else{
    r = solve_list_we(r, e);
  }
  
  h->data.d -= r->data.d;
  
  
#ifdef DEBUG_LISTY
  char res[20];
  char input[20];
  render_list(h, res,   20);
  render_list(l, input, 20);
  printf("(- %s) = (%s)\n", input, res);
#endif

  return h;
}


list *do_div(list *l, env *e){
//  printf("do_div\n");

  if(l == NULL) {
    e->flags |= ENV_ERROR;
    printf("syntax error: division list is empty\n");
  }
  
  if(l->n == NULL){
    printf("do_div warning: l->next is null return %s:%d\n", find_printable_type(l->type), l->data.d);
    return solve_list_we(l, e);
  }
  
  list *h, *r;
  
  /* head */
  h = create_list(l);
  h->n = NULL; // detach list
  h = solve_list_we(h, e);
  
  /* remainder */
  r = l->n;
  if(r && r->n != NULL){
    r = do_div(r, e);
  } else {
    r = solve_list_we(r, e);
  }
  
  if(h->data.d == 0 || r->data.d == 0) {
    printf("do_div error: division by zero (%d / %d)\n", h->data.d, r->data.d);
    e->flags |= ENV_ERROR;
    return NULL;
  }
  
  h->data.d /= r->data.d;
  
  //l    = h;
  //l->n = r;
  
#ifdef DEBUG_LISTY
  char res[20];
  char input[20];
  render_list(h, res,   20);
  render_list(l, input, 20);
  printf("(/ %s) = (%s)\n", input, res);
#endif

  return h;
}

list *do_mul(list *l, env *e){
//  printf("do_mul\n");
  
  if(l == NULL) {
    e->flags |= ENV_ERROR;
    printf("syntax error: multiply list is empty\n");
    return NULL;
  }
  
  if(l->n == NULL){
    printf("do_mul end warning: l->next is null return %s:%d\n", find_printable_type(l->type), l->data.d);
    /* nothing to multiply with, return plain number */  
    return solve_list_we(l, e);
  }
  
  // head, remainder
  list *h, *r;

  /* head */
  h = create_list(l);
  h->n = NULL;
  h = solve_list_we(h, e);
   
  /* remainder */
  r = l->n;
  if(r && r->n != NULL) {
    r = do_mul(r, e);
  }else{
    r = solve_list_we(r, e);
  }
  
  h->data.d *= r->data.d;
  
  l = h;
  
#ifdef DEBUG_LISTY_WS
  char answer[20];
  char input[20];
  render_list(l, input, 20);
  render_list(h, answer, 20);
  printf("(* %s) = (%s)\n", input, answer);
#endif
  return h;
}

void do_senv(list *l, env *e){

  if(l == NULL){
    reply2("error: SENV with no value\n");
    reply2(SENV_help);
    return;
  }

  list *lst = solve_list_we(l, e);

/*
#ifdef DEBUG_LISTY
	reply2("**senv called: ");
	print_list(lst);
	reply2("**senv\n");
#endif
*/

  if(lst->type == TYPE_NUMERIC){
#ifdef DEBUG_LISTY
	reply2("**senv: lst->data.d = %d\n", lst->data.d);
#endif
    switch(lst->data.d){
      case SENV_TRACE:
        e->flags = e->flags^ENV_TRON;
        reply2("set trace %s\n", ((e->flags & ENV_TRON) ? "on" : "off"));
        break; /* boolean toggle */
      case SENV_PMEMLS:
        reply2("\ntotal lists: %d\n", count_all_lists());
        print_all_lists();
        break;
      case SENV_PSTATE:
        print_env(e);
        break;
      case SENV_PPROG:
        reply2("program:\n");
        print_list(lists[0]);
        break;
      case SENV_PNAMES:
        if(lst->n == NULL){
          reply2("defined names: ");
          print_names(e);
          reply2("\n");
        }else{
          if(lst->n->type == TYPE_STRING){
            reply2("name '%s':\n", lst->n->data.s);
            print_list(find_name(lst->n->data.s, e)->body);
          }
          if(lst->n->type == TYPE_NAME){
            reply2("name: %s\n", lst->n->data.n->body->data.s);
            reply2("error: parameter is not a name\n");
          print_list(lst);
          }
        }
        break;
      case SENV_RUN:

        break;
      case SENV_ABORT: /* quit */
        reply2("quit\n");
        e->running = false;
        break;
      default:
        reply2("Invalid parameter\n");
        reply2(SENV_help);
    }/* switch */
  }/* if */
} // void do_senv()

/* reads list and depending upon the format it is:
 * simple name (symbol with empty value)
 * initialized name (symbol with a single list to be replaced on invokation 
 * function name (symbol with two lists:
      the first one containing a simple name or initialized name, 
      and the second one with parsed but not eval'd code
 */
name *do_defun(list *l, env *e){
  reply2("do_defun\n");
  if(l == NULL){
    reply2("defun error list is null.\n");
    return NULL;
    /* TODO: destroy created name */
  }

  if(true){
    print_list(l);
  }

  // l->data.s contains the symbol's name, so far it is an empty symbol
  name *n = create_name(l->data.s); //init name type with the symbol text
  add_name(n, e);                   //add the created name type to the environment 
  
/* holds a defined name string and list value */
//struct name {
//  char  *name; // the string representing the defun'd symbol
//  //char **args; /* maybe could be changed to list **args, which would help type strength */
//  list  *args; // the list pointing to the named args, for local buffer
//  env   *e;    // function internal environment (for local names), must be destroyed on exit.
//  list  *l;    // list pointed to by the name symbol
//};*/

  // the defined name body
  if(l->n){
    list *argv = l->n; //point to actual data
    
    /* TODO: error checking parameters, they should be names */
    if(argv->n != NULL){
      if(argv->n->n) reply2("do_defun warning: list length is longer than expected and was discarded\n");
      n->body = argv->n->data.l; // symbol body
      n->args = argv->data.l;    // symbol parameters
      char txtbody[20];
      render_list(n->body, txtbody, 20); 
      char txtargs[20];
      render_list(n->args, txtargs, 20);
      reply2("function param (%s)\n", txtargs);
      reply2("function body  (%s)\n", txtbody);
      
    } else { //if(argv->n != NULL)
      reply2("do_defun: argv->n is null");
    
      switch(argv->type){
        case TYPE_LIST:{
          char x[20];
          render_list(argv->data.l, x, 20);
          reply2("defined name initializer list (%s)\n", x);
          n->body = argv->data.l; /* the name is initialized, either value or arguments for functionn */
          break;
        }
        
        case TYPE_NUMERIC:
        case  TYPE_STRING:
          reply2("name %s defined as numeric or string\n", n->name);
        break;
      }//switch(argv->type)
    }// argv->n is NULL
  }// second list after the name 

  return n;
}

list *solve_op(list* l, env *e){
  if(l == NULL){
    /* set error ? */
    e->flags |= ENV_ERROR;
    reply2("solve_op error l is null\n");
    return NULL;
  }

  int op = l->data.d;
  list *res = NULL;
  
  switch(op){
    case OP_CMP:
    {
      break;
    }
    case OP_SENV:
      do_senv(l->n, e);
      break;
    case OP_QUO:
      res = l->n;            /* (' X) returns X */
      break;
    case OP_JOIN:
    {
      list *lst;
      res = create_list(l->n); /* copy the first parameter (and the rest of the list) */
      lst = res;
      while(lst->n != NULL){
        lst = lst->n;
      }
      lst->n->n = l->n->n; /* set the second parameter as the tail */
      break;
    }/* OP_JOIN */
    case OP_REST:
      if(l->n && l->n->n)
        res = create_list(solve_list_we(l->n->n, e)); /* (@ (1 2 3)) returns (2 3) */
      break;
    case OP_HEAD:
      if(l->n != NULL){
        res = create_list(
          solve_list_we(l->n, e)
        );              /* (× (1 2 3)) --> (1 2 3)*/
        res->n = NULL;        /* (1 2 3) --> (1) */
      }
      break;
    case OP_DEF:
    {
      l->data.n = do_defun(l->n, e);
      l->type   = TYPE_NAME;
      l->n      = NULL; // discard rest
      res = l;
      //reply2("name defined %s\n", res->data.s);
      
      break;
    }/* OP_DEF */
    case OP_ADD:
      res = do_add(l->n, e);
      l->type = res->type;
      l->data = res->data;
      l->n = NULL;
      
      break;
    case OP_MUL:
    {
      res = do_mul(l->n, e);
      
      l->type   = res->type;
      l->data.d = res->data.d;
      l->n = NULL;
     
      break;
    }
    case OP_DIV:
    {
      res = do_div(l->n, e);
      l->type   = res->type;
      l->data.d = res->data.d;
      l->n      = res->n;
      break;
    }
    case OP_SUB:
    {
      res = do_sub(l->n, e);
      l->type   = res->type;
      l->data.d = res->data.d;
      l->n      = res->n;
      break;
    }

    default:
      reply2("defaulting\n");
      break;
  }/* switch(op) */

#ifdef DEBUG_LISTY
  char x[200];
  render_list(e->program, x, 200);
  reply2("op program %s\n", x);
#endif

  return res;
}

int count_elements(list *l){
  int count = 0;
  while(l != NULL){
    l = l->n;
    count++;
  }
  return count;
}

void print_name(name *n){
  if(n != NULL){
    char *s;
    int i = 0;
    char txtbody[20];
    char txtargs[20];
    
    render_list(n->args, txtargs, 20);
    render_list(n->body, txtbody, 20);
    reply2("name %s:\n  args: %s\n  body: %s\n", n->name, txtargs, txtbody);    
  }else{
    reply2("error: name is null\n");
  }
}

void print_type(list *l){
  if(l != NULL) reply2(find_printable_type(l->type)); 
  return;
}

void print_value(list *l){
  if(l != NULL){
    int t = l->type;
    switch(l->type){
      case TYPE_NULL:    reply2("NULL");          break;
      case TYPE_NUMERIC: reply2("%d", l->data.d); break;
      case TYPE_OP: {
        if(l->data.d == OP_SENV){
          reply2("ESC");
        }else{
          reply2("%c", l->data.d);
        }
         break;
      }
      case TYPE_STRING: reply2("{%s}", l->data.s); break;
      case TYPE_SYMBOL: reply2("%s", l->data.s); break;

      case TYPE_NAME: {
        name *n = l->data.n;
        if(n == NULL) {reply2("name is null\n"); break;}

        //if(l->data.n->name == NULL) {reply2 ("data.n->name is null\n");}
        if(n->name != NULL) reply2("name: %x", n->name);
        if(n->args != NULL) reply2("args: %x", n->args);
        if(n->body != NULL) print_list_wd(n->body, 1);
        break;
      }
      case TYPE_LIST: {
        if(l->data.l != NULL){
          reply2("(list %d)", find_list(l->data.l));
        }else{
          reply2("NULL");
        }
        break;
      }/* if(t == TYPE_LIST) */
    } //switch (l->type) 
  } // if l != NULL
}// print_value( list )

void print_names(env *e){
  int n_len = e->names.used;
  name *n = NULL;
  int i = 0;
  for(i = 0; i != n_len; i++){
    n = e->names.ptr[i];
    reply2("%s ", n->name);
  }
}

int find_list(list *l){
  int i = 0;
  for(i = 0; i != MAX_LEN; i++){
    if(l == lists[i]) return i;
  }
  return -1;
}

int print_list_count = 0;
void print_list(list *l){
  print_list_count = 0;
  reply2("list (\n");
  print_list_wd(l, 0);
  reply2(")\n");

}

/* prints a human readable list using proper indentation, one element per line */
void print_list_wd(list *l, int depth){
  int i = 0;
  depth++;
  if(l == NULL){
    for(i = 0; i != depth; i++) putchar('.');
    reply2("0: null\t()\n");
    return;
  }

  if(depth >= 16){
    reply2("Too much depth!\n");
    return;
  }

  int n = 0;

  while(l != NULL){
    n = print_list_count;

    if(l == l->n){
      reply2("error: list points to itself\n");
      return;
    }
    
    int t;
    t = l->type;

    reply2("%3d: ", n);
    for(i = 0; i != depth; i++) putchar('.');
    print_type(l);
    reply2("   \t");

    if(t != TYPE_LIST){
      print_value(l);
      reply2("\n");

    /* recurse if data is another list */
    }else{
      reply2("\n");
      print_list_count++;
      if(l->data.l != NULL) print_list_wd(l->data.l, depth+1);
      print_list_count--;
    }

    l = l->n;
    
    n++;
    print_list_count++;
  } /* while(l != NULL) */
}

int iswspc(char c){
  if(c == ' ' ) return true;
  if(c == '\t') return true;
  if(c == '\n') return true;
  if(c == '(' ) return true;
  if(c == ')' ) return true;
  if(c ==  0  ) return true;
  return false;
}


//TODO: read hex characters as numbers
int isnum(char c){
  if(c >= '0' && c <= '9') return true;
  return false;
}


/*
figures out what kind of data is being read from the code string 
advances e->ptr on read
*/
int get_type(env *e){

  int type = TYPE_NULL;

  //current position as set in the environment
  char *c = e->code+e->ptr;

  if(e->ptr > e->len)
    return TYPE_NULL;

  /* read c, depending on its value it will determine the type of data being read */
  switch(*c){

    //for primitive operators, they all are single character (except for unicode... must verify this)
    case OP_DEF:   // %
    case OP_SENV:  // 
    case OP_CMP:   // ?
    case OP_GT:    // >
    case OP_LT:    // <
    case OP_EQ:    // = 
    case OP_NEQ:   // !
    case OP_QUO:   // '
    case OP_HEAD:  // .
    case OP_REST:  // @
    case OP_JOIN:  // &
    case OP_ADD:   // +
    case OP_MUL:   // *
    case OP_DIV:   // /
    case OP_SUB:   // -
      type = TYPE_OP;
      e->ptr++;
      break;

//    case OP_SUB: /* allow use of negative numbers */
//      if(iswspc(*(c+1))){
//        type = TYPE_OP;
//        e->ptr++;
//      }else{
//        /* type is numeric negative ?*/
//      }

    /* data blocks (strings) */
    case S_OPEN: { // {
      type = TYPE_STRING;
      while(e->ptr <= e->len){
        if(*c == S_CLOSE) break;
        c++;
        e->ptr++;
      }/* while */
      break;
    } // S_OPEN
    case S_CLOSE: {// }
      e->ptr++;
      break;
    } // S_CLOSE
    
    /* lists */ 
    case L_CLOSE:  // )
      e->ptr++;
      break;      
    case L_OPEN: { // (
      type = TYPE_LIST;
      int open = 0;
      while(e->ptr <= e->len){
        //putchar(*c);
        if(*c == L_CLOSE) open--;
        if(*c == L_OPEN) open++;
        if(open == 0) break;
        c++;
        e->ptr++;
      }/* while */
      break;
    } //L_CLOSE
    
    /* white space being ignored */
    case '\t':
    case '\n':
    case ' ':
      e->ptr++;
      break;
      
    default:
      // ignore white space 
      if(iswspc(*c)){
        e->ptr++;
        break;
      }/* if(iswspc(*c)) */
      
      /* read numeric input (0123456789) */
      if(isnum(*c)){
        type = TYPE_NUMERIC;
        
        //read until whitespace is found
        while(!iswspc(*c)){
          if(!isnum(*c))
            type = TYPE_NAME;
          c++;
          e->ptr++;
        }/* while(!isnum()) */
      
      /* if its not numeric it must be a name az-AZ */  
      // TODO: make a validation of name string
      }else{ /* if(isnum(*c)) */
        type = TYPE_SYMBOL;
        
        // read until whitespace is found 
        while(!iswspc(*c) && e->ptr <= e->len){
          c++;
          e->ptr++;
        } /* while(!iswspc(*c) && e->ptr <= e->len) */
      } /* else */
    }/* switch(*c) */

  /*
  if(type != TYPE_NULL)
    reply2("%03d:%03d -> %02d \"%c%c\"\n", start, e->ptr, type, e->code[start], e->code[start+1]);
  */
  return type;
}

int count_all_lists(){
  int i = 0, count = 0;
  for(i = 0; i != MAX_LEN; i++){
    if(lists[i] != NULL) count++;
  }
  return count;
}

void print_all_lists(){
  int i = 0;
  for(i = 0; i != MAX_LEN; i++){
    if(lists[i] != NULL){
      reply2("list[%2d].type: ", i);
      print_type(lists[i]);
      reply2("\t(");
      print_value(lists[i]);
      reply2(")\n");
    }
  }
  reply2("\n");
}

int get_numeric(char *c){
  int   res;

  /* transform to int */
  res = atoi(c);

  return res;
}

char* get_string(char *c){
  char *res;
  int len = 0;
  while(c[len] != S_CLOSE)
    len++;

  res = strndup(c+1, len-1); /* c+1 removes block opening */
  return res;
}

char* get_name(char *c){
  char *res;
  int len = 0;

  while(!iswspc(c[len]))
    len++;

  res = strndup(c, len);
  //reply2("symbol %s\n", res);
  return res;
}

/*
 * finds the next available marking for type
 * type = the type of marking to find
 * returns the first available marking or MAX_LEN if none is available
 */
int get_next_mark(int type){
  int i = 0;
  for(i = 0; i != MAX_LEN; i++){
    if((marks[i] & type) == 0){
      //reply2("type: %d\ni   : %d\n", type, i);
      return i;
    }
  }
  return MAX_LEN; /* Can't find free mark */
}

/*
 * The marks array holds information of the other (envs, lists, names) arrays
 * and whether they do or not contain a previously allocated piece in each index
 * each bitmask in marks[] holds information for each types in each bit.
 * for example:
 * marks[x] = 0 0 0 0   0 1 0 1 - there are 2 types which are malloc'd at x index.
 * each TYPE_* defines one single bit to be used as a mask.
 */

/*
 * sets the marking for type on the specified index
 * index = index to mark
 * type  = type of marking
 * returns true if the marking could be set; false otherwise.
 */
int set_mark(int index, int type){
  if(index <= MAX_LEN){
    if((marks[index] & type) != type){ /* flag in marks[index] is 0 */
      marks[index] |= type; /* set */
      return true;
    }
  }
  return false;
}

/*
 * clears markings
 */
int clear_mark(int index, int type){
  if(index <= MAX_LEN){
    if((marks[index] & type) == type){ /* flag in marks[index] is 1 */
      marks[index] &= ~type; /* clear */
      return true;
    }
  }
  return false;
}

name *create_name(char *text){
  if(!text){
    reply2("create_name error: name text is null\n");
    return NULL;
  }
  
  name *n = malloc(sizeof(name));
  int len = strlen(text);
  
  n->name = malloc(len);
  strncpy(n->name, text, len);
  reply2("create_name: '%s' \n", n->name);
  
  /* use the next available slot in the buffer for storing this name */
  int mark_index = get_next_mark(TYPE_NAME);
  if(mark_index != MAX_LEN){
    set_mark(mark_index, TYPE_NAME);
    names[mark_index] = n;
    reply2("create_name: '%s' stored at position %d\n", n->name, mark_index);
  }else{
    /* resize */
    reply2("No available space for new name!\n");
  }
  
  return n;
}

void free_env(env *e){
  int mark_index;

  for(mark_index = 0; mark_index != MAX_LEN; mark_index++){
    if(envs[mark_index] == e){ /*pointer to pointer comparision */
      free(envs[mark_index]); /* free() should do nothing if it's NULL pointer */
      clear_mark(mark_index, TYPE_ENV);
      envs[mark_index] = NULL; /* not really needed */
    }
  }
}

void free_list(list *l){
  int mark_index;
  if(l != NULL){
    for(mark_index = 0; mark_index != MAX_LEN; mark_index++){
      if(lists[mark_index] == l){ /* pointer to pointer comparision */
        free(lists[mark_index]); /* free should do nothing if it's NULL ptr */
        clear_mark(mark_index, TYPE_LIST);
        lists[mark_index] = NULL; /* not really needed */
      }
    }
  }
}


// struct env{
//   char *code; /*                                                */
//   int   len;  /* this three elements are not needed for evaling */
//   int   ptr;  /*                                                */
//   list *program;
//   
//   struct {
//     name **ptr;
//     int    len;
//     int    used;
//   } names;
//   
//   env *parent;
//   int  flags;
//   int  running;
// }; /* struct env */

/* print environment data */
void print_env(env *e){
  reply2("** environment start\n> code(%d): \"%s\"\n> names(%d): used: %d ptr: x%08x\n> parent:   %x\n> trace:    %s\n",
    e->ptr,
    e->code,
    e->names.len,
    e->names.used,
    (unsigned long)e->names.ptr,
    (unsigned long)e->parent,
    ((e->flags & ENV_TRON) ? "on" : "off")
  );
  reply2("> program(%s): flags: %04x length: %d\n",
    (e->running == true ? "running" : "stopped"),
    e->flags,
    count_elements(e->program)
  );
  reply2("** environment end\n");
}

env *create_env(env *origin){
#ifdef DEBUG_LISTY
  reply2("** create env %s\n", (origin == NULL ? "empty" : "with origin"));
#endif
  env *en = malloc(sizeof(env));

  int mark_index = get_next_mark(TYPE_ENV);
  if(mark_index != MAX_LEN){
    set_mark(mark_index, TYPE_ENV);
    envs[mark_index] = en;
  }else{
    /* resize */
    reply2("No available space for new environment!\n");
  }

  if(origin == NULL){ /* creating an empty environment */
    en->code      = NULL;
    en->len       = 0;
    en->ptr       = 0;
    en->names.len = 0;
    en->names.used= 0;
    en->names.ptr = NULL;
    en->parent    = NULL;
    en->flags     = 0;
    en->running   = false;
  }else{              /* is duplicating */
    en->code      = origin->code;
    en->len       = origin->len;
    en->ptr       = origin->ptr;
    en->names.len = origin->names.len;
    en->names.ptr = origin->names.ptr; /* should copy the names ? */
    en->names.used= origin->names.used;
    en->parent    = origin->parent;
    en->flags     = 0; /* always off unless specifically set on */
    en->running   = false;
  }
  return en;
}

/* creates numeric list */
list *create_list_numeric(int d){
  list *l = create_list(NULL);
  l->type = TYPE_NUMERIC;
  l->data.d = d;
  return l;
}

list *set_list_numeric(list *l, int d){
  l->type = TYPE_NUMERIC;
  l->data.d = d;
  return l;
}

list *copy_list(list *l, list *src){
  if(src == NULL)
    return l; /* nothing to copy */

  if(l == NULL){
    l = create_list(src);
  }else{
    l->type = src->type;
    l->data = src->data;
  }
  return l;
}

/* malloc's and sets a list data */
list *create_list(list *origin){
  list *l = malloc(sizeof(list));

  int mark_index = get_next_mark(TYPE_LIST);
  
#ifdef DEBUG_LIST_WS
  printf("create_list %3d %s %d\n",
      mark_index, 
      (origin ? find_printable_type(origin->type) : "empty"), // type
      (origin ? origin->data.d : 0)
  );
#endif

  if(mark_index != MAX_LEN){
    set_mark(mark_index, TYPE_LIST);
    lists[mark_index] = l;
  }else{
    /* should resize the buffer here */
    reply2("No available space for new list!\n");
  }

  if(origin == NULL){ /* creating an empty list */
    l->data.d = 0;
    l->type   = TYPE_NULL;
    l->n      = NULL;
  }else{              /* is duplicating */
    l->data   = origin->data;
    l->type   = origin->type;
    l->n      = origin->n;
  }
  return l;
}

int init_env(env *e, char *code){
#ifdef DEBUG_LISTY
    //reply2("** init_env code: \"%s\"\n", code);
#endif // DEBUG_LISTY

  // probably unneccesary, who knows 
  memset(marks, 0, MAX_LEN);

  if(code != NULL){
    e->code = code;
    e->ptr  = 0;
    e->len  = strlen(code)-1; /* disregard ending \0 */
  }
  return 0;
}


/* loads string of text as a list, parsing every part as their corresponding list representation */
list *load_we(env *e){

  char *code = e->code; /* pointer to the char* where code begins */
  int   ptr  = e->ptr;  /* offset in chars from code beings up to processed position */
  char  c;              /* character being read */
  
  /* this will read text up to the closing of the list, converting the char* into the proper listy data type 
   * parsing character by character until the list is closed (maybe)
   */
  list     *head = create_list(NULL); /* resulting list being processed */
  list     *l    = head;              /* the active node being processed into */  
  list_type t    = TYPE_NULL;         /* the type of the list data being processed */
  

//#ifdef DEBUG_LISTY
//  reply2("** load_we code(%d): \"%s\" \n", ptr, code+(ptr == 0? 0 : ptr-1) );
//#endif

  // loops each character in code using ptr as the reading position
  while(ptr <= e->len){
    ptr = e->ptr;      /* local copy of environment execution pointer, before reading by get_type  */
    c   = code[ptr];   /* pointer to code string */
    t   = get_type(e); /* the listy type being read in environment advancing e->ptr */

#ifdef DEBUG_LISTY_WS
  reply2("** load_we c=%c, t=%s \n", c, find_printable_type(t) );
#endif

    //#define L_CLOSE     ')'  /* ends list */
    if(c == L_CLOSE){
      break;
    }

    switch(t){
    
      //#define TYPE_LIST    4
      case TYPE_LIST: {
        /* moves to the next character */
        e->ptr = ptr+1;
        list *nl = load_we(e);

        /* if the list is already used, create a new one */
        if(l->type != TYPE_NULL){
          l->n = create_list(NULL);
          l = l->n;
        }

        l->data.l = nl; /* set the returned list as this list's data */
        l->type = TYPE_LIST;
        break;
      }/* case TYPE_LIST */
      
      //#define TYPE_NUMERIC 1
      case TYPE_NUMERIC: {
        list *nl = create_list(NULL);

        nl->data.d = get_numeric(&code[ptr]);
        nl->type = TYPE_NUMERIC;
        /* there is no oper; list is pure (or semi-pure) */
        if(head->type == TYPE_NULL){
          free_list(head);
          head = nl;
          l = head;
        }else{
          l->n = nl;
          l = nl;
        }
        break;
      }/* case TYPE_NUMERIC */
      
      //#define TYPE_STRING  2
      case TYPE_STRING:
      {
        list *nl = create_list(NULL);
        char* block = get_string(&code[ptr]);

        nl->data.s = block;
        nl->type = TYPE_STRING;

        if(head->type == TYPE_NULL){
          free_list(head);
          head = nl;
          l = head;
        }else{
          l->n = nl;
          l = nl;
        }
        break;
      }/* case TYPE_STRING */
      
      //#define TYPE_NAME    8
      case TYPE_NAME: {
        // (% a-zA-Z0-9)
        char* name = get_name(&code[ptr]); //copy code string into the names array, keeping a pointer to the loaded symbol
        list *nl   = create_list(NULL);    // list holding the listy representation of name (not solved)
        nl->data.s = name;                 // char* string holding the name
        nl->type   = TYPE_NAME;
        
        //if the list being read has a name as the operator 
        if(head->type == TYPE_NULL){
          head->type   = TYPE_NAME;
          head->data.l = nl;
          head->n      = NULL;
          reply2("head->type is name\n");
        }else{
          /* the name is within a list to be processed as a symbolic reference */
          l->n = nl;
          l = nl;
          reply2("l is name %s\n", l->data.s);
        } // if(head->type == TYPE_NULL) 

        break;
      }/* case TYPE_NAME */
      
      //#define TYPE_SYMBOL 10 /* a name defined without defun */
      case TYPE_SYMBOL: {
        char *symbol = get_name(&code[ptr]);
        //reply2("symbol %s\n", symbol);
        // search for existing symbol, if found use it, otherwise, define it
        list *nl;
        
        if(head->type == TYPE_NULL){ // the first element is a symbol
          head->data.s = symbol;
          head->type   = TYPE_SYMBOL;
        } else { // symbol is found in the middle of a list
          nl = create_list(NULL);
          nl->data.s = symbol;
          nl->type   = TYPE_SYMBOL;
          l->n = nl;
          l = nl;
        } // if head->n == NULL
        
        //reply2("head is %d; nl is %d; l is %d\n", head, nl, l );
                
        break;
      }
      //#define TYPE_OP     32
      case TYPE_OP: {
        /* sets head type as oper (as opossed to data-list)
         * modifies type+data on head list
         */       
        if(head->n == NULL) { //op is heading the list
          head->data.d = code[ptr];
          head->type   = TYPE_OP;
        }else{
          reply2("load warning op out of place\n");
        }
        break;
      }/* case TYPE_OP */      
    }/* switch(t) */
  }/* while(e->ptr <= e->len) */
  
#ifdef DEBUG_LISTY_WS
  reply2("** load_we head start\n");
  print_list(head);
  reply2("** load_we head end\n");
#endif
  return head;
}


/*
parses text in code converting it into a listy representation 
creates empty environment
returns the environent loadeded with the internal listy representation of char *code 
*/
env *load(char *code){
  env *e = create_env(NULL);
  init_env(e, code);
  reply2("e->code: %s\n", code);
  e->program = load_we(e);
  
#ifdef DEBUG_LISTY
  char program[200];
  printable_src = 1;
  render_list(e->program, program, 200);
  reply2("** loaded program:\n%s\n", program);
#endif
  return e;
}

int printlen(char *c){
  int l = strlen(c);
  char *hex = "0123456789";
  int i = 0;

  putchar('[');
  for(i = 0; i != l; i++){
    if((i%10) == 0){
      putchar(hex[i/10]);
    }else{
      putchar(' ');
    }
  }
  putchar(']');
  putchar('\n');

  putchar('[');
  for(i = 0; i != l; i++){
    putchar(hex[i%10]);
  }
  putchar(']');
  putchar('\n');

  return 0;
}

list *eval(char *code){
  env *e = load(code);
#ifdef DEBUG_LISTY_WS
  reply2("** eval env start\n");
  print_env(e);
  reply2("** eval env end\n");
#endif
  reply2("run\n");
  e->running = true;
  return solve_list_we(e->program, e);
}


/*int main (int argc, char **argv){
  char buffer[1024] = "(* (+ 1 2) 3)";
  reply2("code: %s\n", buffer);

  print_list(eval(buffer));

  return 0;
}
*/
