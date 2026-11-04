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
 * nil@nil)
 *
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