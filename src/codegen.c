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
 * nil@nil) and arguments are read from stack in reverse order.
 *
 * @important Built-in function handling:
 *  inline:
 *      readLine(): string
 *      toInt(val: Double): Int
 *      strLength(s: String): Int
 *      chr(code: Int): String
 *  extra func:
 *      print(val: Any): Unit
 *      toStr(val: Any): String
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

void cg_init() {
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

  // $builtin_print
  fputs("LABEL $builtin_print\n"
        "POPS GF@$tmp1\n"
        "JUMPIFEQ $print_unit GF@$tmp1 nil@nil\n"
        "WRITE GF@$tmp1\n"
        "JUMP $print_end\n"

        "LABEL $print_unit\n"
        "WRITE string@()\n"

        "LABEL $print_end\n"
        "PUSHS nil@nil\n"
        "RETURN\n",
        stdout);
}