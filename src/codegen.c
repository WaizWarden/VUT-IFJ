/**
 * @file    codegen.c
 * @author  xoramst00
 * @brief   Generator implementation of final code IFJcode26
 *
 * @important Code generator passes through the tree twice:
 *      1. pre-pass: finds all possible DEFVAR
 *      2. print-pass: instructions are automatically written to stdout
 *      (this helps to avoid using dynamic buffer for adding DEFVAR mid pass)
 *
 * @important All helper variables and identifiers start with '$' or '%' to
 * solve collisions with user code global variables: GF@$tmp user functions:
 * $func_... built-in functions: $builtin_... control identifiers: $if_else_...
 *      local variables: LF@jmeno$[uid]
 *
 * @important Calling convencion: RETURN VALUE IS LEFT ON STACK TOP (even
 * nil@nil) and arguments are read from stack in reverse order. (last arg on top)
 *
 * @important Built-in function handling:
 *  inline:
 *      readLine(): string
 *      toInt(val: Double): Int
 *      strLength(s: String): Int
 *      chr(code: Int): String
 *      toStr(value: Any): String
 *      print(value: Any): Unit
 *  extra func:
 *      readInt(): Int
 *      readDouble(): Double
 *      ord(s: String, i: Int): Int
 *      substr(s, start, end)
 */

// defined up to characters that have to be printed as /xyz like ' '
#define SPECIAL_CHAR_IN_STRING_MAX 32

#include "codegen.h"
#include <stdio.h>
#include <string.h>

// recomended by ai to make counter global to simplify architecture
static uint32_t label_counter = 0;

// use for fast jumps in if/else/<=...
uint32_t cg_next_label_id() { return label_counter++; }

void cg_print_int(int64_t val) { printf("int@%lld", (long long)val); }

void cg_print_double(double val) { printf("float@%a", val); }

void cg_print_bool(bool val) {
  val ? printf("bool@true") : printf("bool@false");
}

void cg_print_unit(void) { printf("nil@nil"); }

void cg_print_string(char *str) {

  char *nextChar = str;

  printf("string@");
  while (nextChar && *nextChar != '\0') {

    // escape characters are already parsed to one character
    unsigned char sym = (unsigned char)*nextChar;

    if (sym <= SPECIAL_CHAR_IN_STRING_MAX || sym == '#' || sym == '\\') {
      printf("\\%03d", sym);
    } else {
      putchar(sym);
    }

    nextChar++;
  }
}

void cg_print_var(const char *name, uint32_t uid) {
  printf("LF@%s$%u", name, uid);
}

void cg_gen_head() {
  label_counter = 0;

  // print unified start of code ---
  fputs(".IFJcode26\n"
        "DEFVAR GF@$tmp1\n"
        "DEFVAR GF@$tmp2\n"
        "DEFVAR GF@$tmp3\n"
        "CALL $main\n"
        "EXIT int@0\n",
        stdout);

  // print all built-in functions ---

  // $builtin_readInt and $builtin_readDouble
  fputs("LABEL $builtin_readInt\n"
        "READ GF@$tmp1 int\n"
        "JUMPIFNEQ $readInt_ok GF@$tmp1 nil@nil\n"
        "MOVE GF@$tmp1 int@0\n"

        "LABEL $readInt_ok\n"
        "PUSHS GF@$tmp1\n"
        "RETURN\n",
        stdout);
  fputs("LABEL $builtin_readDouble\n"
        "READ GF@$tmp1 float\n"
        "JUMPIFNEQ $readDouble_ok GF@$tmp1 nil@nil\n"
        "MOVE GF@$tmp1 float@0x0p+0\n"

        "LABEL $readInt_ok\n"
        "PUSHS GF@$tmp1\n"
        "RETURN\n",
        stdout);

  // $builtin_ord (value string, index int) - int on stack top (revised by gemini)
  fputs("LABEL $builtin_ord\n"
        "POPS GF@$tmp2\n"
        "POPS GF@$tmp1\n"

        "LT GF@$tmp3 GF@$tmp2 int@0\n" 
        "JUMPIFEQ $ord_err GF@$tmp3 bool@true\n"  // index < 0

        "STRLEN GF@$tmp3 GF@$tmp1\n"
        "LT GF@$tmp3 GF@$tmp2 GF@$tmp3\n"
        "JUMPIFEQ $ord_err GF@$tmp3 bool@false\n" // index >= len

        "STRI2INT GF@$tmp1 GF@$tmp1 GF@$tmp2\n"
        "PUSHS GF@$tmp1\n"
        "RETURN\n"

        "LABEL $ord_err\n"
        "PUSHS int@-1\n"
        "RETURN\n",
    stdout);

  // $builtin_substr (value string, start int, end int) (architecture by gemini)
  fputs("LABEL $builtin_substr\n"

        // create new local frame and define all helper vars
        "CREATEFRAME\n"
        "DEFVAR LF@str\n"
        "DEFVAR LF@start\n"
        "DEFVAR LF@end\n"
        "DEFVAR LF@len\n"
        "DEFVAR LF@res\n"
        "DEFVAR LF@ch\n"
        "DEFVAR LF@cond\n"
        "PUSHFRAME\n"
        "POPS LF@end\n"
        "POPS LF@start\n"
        "POPS LF@str\n"

        "STRLEN LF@len LF@str\n"

        // checks parameter validity
        "LT LF@cond LF@start int@0\n"
        "JUMPIFEQ $substr_err40 LF@cond bool@true\n"  // start < 0
        "LT LF@cond LF@end int@0\n"
        "JUMPIFEQ $substr_err40 LF@cond bool@true\n"  // end < 0
        "GT LF@cond LF@start LF@end\n"
        "JUMPIFEQ $substr_err40 LF@cond bool@true\n"  // start > end
        "LT LF@cond LF@start LF@len\n"
        "JUMPIFEQ $substr_err40 LF@cond bool@false\n" // start >= len
        "GT LF@cond LF@end LF@len\n"
        "JUMPIFEQ $substr_err40 LF@cond bool@true\n"  // end < len

        // loop over string, concat to @res ultil reaching 'end'
        "MOVE LF@res string@\n"
        "LABEL $substr_loop\n"
        "JUMPIFEQ $substr_done LF@start LF@end\n"
        "GETCHAR LF@ch LF@str LF@start\n"
        "CONCAT LF@res LF@res LF@ch\n"
        "ADD LF@start LF@start int@1\n"
        "JUMP $substr_loop\n"

        // clean frame and return value
        "LABEL $substr_done\n"
        "PUSHS LF@res\n"
        "POPFRAME\n"
        "RETURN\n"
        "LABEL $substr_err40\n"
        "EXIT int@40\n",
  stdout);

  /// @author xsirmat00 from here -----------------------------------------------

}