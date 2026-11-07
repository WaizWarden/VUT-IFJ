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
 * nil@nil) and arguments are read from stack in reverse order. (last arg on
 * top)
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
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// recomended by ai to make counter global to simplify architecture
static uint32_t label_counter = 0;

// use for fast jumps in if/else/<=...
uint32_t cg_next_label_id(void) { return label_counter++; }

void cg_print_int(int64_t val) { printf("int@%" PRId64, val); }

void cg_print_double(double val) { printf("float@%a", val); }

void cg_print_bool(bool val) {
  val ? printf("bool@true") : printf("bool@false");
}

void cg_print_unit(void) { printf("nil@nil"); }

void cg_print_string(const char *str) {

  const char *nextChar = str;

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

void cg_gen_head(void) {
  label_counter = 0;

  // print unified start of code ---
  puts(".IFJcode26\n"
       "DEFVAR GF@$tmp1\n"
       "DEFVAR GF@$tmp2\n"
       "DEFVAR GF@$tmp3\n"
       "CALL $main\n"
       "EXIT int@0\n");

  // print all built-in functions ---

  // $builtin_readInt and $builtin_readDouble
  puts("LABEL $builtin_readInt\n"
       "READ GF@$tmp1 int\n"
       "JUMPIFNEQ $readInt_ok GF@$tmp1 nil@nil\n"
       "MOVE GF@$tmp1 int@0\n"

       "LABEL $readInt_ok\n"
       "PUSHS GF@$tmp1\n"
       "RETURN\n");
  puts("LABEL $builtin_readDouble\n"
       "READ GF@$tmp1 float\n"
       "JUMPIFNEQ $readDouble_ok GF@$tmp1 nil@nil\n"
       "MOVE GF@$tmp1 float@0x0p+0\n"

       "LABEL $readDouble_ok\n"
       "PUSHS GF@$tmp1\n"
       "RETURN\n");

  // $builtin_ord (value string, index int) - int on stack top (revised by
  // gemini)
  puts("LABEL $builtin_ord\n"
       "POPS GF@$tmp2\n"
       "POPS GF@$tmp1\n"

       "LT GF@$tmp3 GF@$tmp2 int@0\n"
       "JUMPIFEQ $ord_err GF@$tmp3 bool@true\n" // index < 0

       "STRLEN GF@$tmp3 GF@$tmp1\n"
       "LT GF@$tmp3 GF@$tmp2 GF@$tmp3\n"
       "JUMPIFEQ $ord_err GF@$tmp3 bool@false\n" // index >= len

       "STRI2INT GF@$tmp1 GF@$tmp1 GF@$tmp2\n"
       "PUSHS GF@$tmp1\n"
       "RETURN\n"

       "LABEL $ord_err\n"
       "PUSHS int@-1\n"
       "RETURN\n");

  // $builtin_substr (value string, start int, end int) (architecture by gemini)
  puts("LABEL $builtin_substr\n"

       // create new local frame and define all helper vars
       "CREATEFRAME\n"
       "PUSHFRAME\n"
       "DEFVAR LF@str\n"
       "DEFVAR LF@start\n"
       "DEFVAR LF@end\n"
       "DEFVAR LF@len\n"
       "DEFVAR LF@res\n"
       "DEFVAR LF@ch\n"
       "DEFVAR LF@cond\n"
       "POPS LF@end\n"
       "POPS LF@start\n"
       "POPS LF@str\n"

       "STRLEN LF@len LF@str\n"

       // checks parameter validity
       "LT LF@cond LF@start int@0\n"
       "JUMPIFEQ $substr_err40 LF@cond bool@true\n" // start < 0
       "LT LF@cond LF@end int@0\n"
       "JUMPIFEQ $substr_err40 LF@cond bool@true\n" // end < 0
       "GT LF@cond LF@start LF@end\n"
       "JUMPIFEQ $substr_err40 LF@cond bool@true\n" // start > end
       "LT LF@cond LF@start LF@len\n"
       "JUMPIFEQ $substr_err40 LF@cond bool@false\n" // start >= len
       "GT LF@cond LF@end LF@len\n"
       "JUMPIFEQ $substr_err40 LF@cond bool@true\n" // end < len

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
       "EXIT int@40\n");
}

/// @brief Passes through whole tree and generates all var declarations,
/// recursive
/// @param node ast of subprogram
static void cg_gen_vars_prepass(const AstNode *node) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case AST_VAR_DECL:
    fputs("DEFVAR ", stdout);
    cg_print_var(node->as.varDecl.name, node->uid);
    putchar('\n');
    cg_gen_vars_prepass(node->as.varDecl.initExpr);
    break;

  case AST_BLOCK:
    for (size_t i = 0; i < node->as.block.count; i++) {
      cg_gen_vars_prepass(node->as.block.stmts[i]);
    }
    break;

  case AST_IF:
    cg_gen_vars_prepass(node->as.ifStatement.thenBlock);
    cg_gen_vars_prepass(node->as.ifStatement.elseBlock);
    break;

  case AST_WHILE:
    cg_gen_vars_prepass(node->as.whileStatement.bodyBlock);
    break;

  case AST_ASSIGN:
    cg_gen_vars_prepass(node->as.assign.value);
    break;

  case AST_RETURN:
    cg_gen_vars_prepass(node->as.ret.expr);
    break;

  default: // literals, function calls and othes do not require a pass
    break;
  }
}

/// @brief Prints a function definition
/// @param node ast tree of function
static void cg_gen_func_def(const AstNode *node) {
  if (!node) {
    return;
  }

  if (node->as.funcDef.isMain) {
    puts("LABEL $main\n");
  } else {
    printf("LABEL $func_%s\n", node->as.funcDef.name);
  }

  puts("CREATEFRAME\n"
       "PUSHFRAME\n");

  // declare internal parameters
  for (uint16_t i = 0; i < node->as.funcDef.paramCount; i++) {
    fputs("DEFVAR ", stdout);
    cg_print_var(node->as.funcDef.params[i].name,
                 node->as.funcDef.params[i].uid);
    putchar('\n');
  }

  // load parameter values from stack from right to left
  for (int i = (int)node->as.funcDef.paramCount - 1; i >= 0; i--) {
    fputs("POPS ", stdout);
    cg_print_var(node->as.funcDef.params[i].name,
                 node->as.funcDef.params[i].uid);
    putchar('\n');
  }

  // declare internal variables
  cg_gen_vars_prepass(node->as.funcDef.body);

  // generate function body
  cg_gen_block(node->as.funcDef.body, true, node->as.funcDef.retType);

  puts("POPFRAME\n"
       "RETURN");
}

/// @brief Helper function to get correct return type (implicit conversions and
/// unit)
/// @param exprType expresion type
/// @param funcRetType return type
static void cg_adjust_return_value(DataType exprType, DataType funcRetType) {

  if (funcRetType == T_UNIT) {
    // no matter the value, return nil@nil
    puts("POPS GF@$tmp1\n"
         "PUSHS nil@nil");
  }

  if (funcRetType == T_DOUBLE && exprType == T_INT) {
    puts("INT2FLOATS");
  }
}

void cg_gen_statement(const AstNode *stmt, DataType funcRetType) {
  if (!stmt) {
    return;
  }

  switch (stmt->type) {

  case AST_VAR_DECL: // var / val ...
    cg_gen_expr(stmt->as.varDecl.initExpr);

    if (stmt->as.varDecl.hasDeclaredType &&
        stmt->as.varDecl.declaredType == T_DOUBLE &&
        stmt->as.varDecl.initExpr->evalType == T_INT) {
      puts("INT2FLOATS");
    }

    // save stack top to var (declared by prepass of var declarations)
    fputs("POPS ", stdout);
    cg_print_var(stmt->as.varDecl.name, stmt->uid);
    putchar('\n');
    break;

  case AST_ASSIGN: // thing = expression
    cg_gen_expr(stmt->as.assign.value);

    // check type
    if (stmt->evalType == T_DOUBLE &&
        stmt->as.assign.value->evalType == T_INT) {
      puts("INT2FLOATS");
    }

    // save return val to var
    fputs("POPS ", stdout);
    cg_print_var(stmt->as.assign.name, stmt->uid);
    putchar('\n');
    break;

  case AST_RETURN: // retunr expression

    if (stmt->as.ret.expr == NULL) {
      puts("PUSHS nil@nil");
    } else {
      cg_gen_expr(stmt->as.ret.expr);
      cg_adjust_return_value(stmt->as.ret.expr->evalType, funcRetType);
    }

    puts("POPFRAME\n"
         "RETURN");
    break;

  case AST_IF: // if(horsingAround)
    cg_gen_if(stmt, false, funcRetType);
    break;

  case AST_WHILE: // while(horsingAround)
    cg_gen_while(stmt, funcRetType);
    break;

  case AST_EXPR_STATEMENT: // isHorsingAround(horse)
    cg_gen_expr(stmt->as.exprStatement.expr);
    puts("POPS GF@$tmp1"); // not at end of block: return not needed
    break;

  case AST_FUNC_CALL: // parser puts fuction into block instead of as statement
                      // type
    cg_gen_call(stmt);
    puts("POPS GF@$tmp1"); // not needed
    break;

  default: // parser puts expression as a command
    cg_gen_expr(stmt);
    puts("POPS GF@$tmp1"); // not needed
    break;
  }
}

void cg_gen_block(const AstNode *block, bool isLastInFunc,
                  DataType funcRetType) {
  if (!block) {
    return;
  }

  if (block->type != AST_BLOCK) {
    const AstNode *expr = (block->type == AST_EXPR_STATEMENT)
                              ? block->as.exprStatement.expr
                              : block;
    if (isLastInFunc) {
      cg_gen_expr(expr);
      cg_adjust_return_value(expr->evalType, funcRetType);
    } else {
      cg_gen_statement(block, funcRetType);
    }
    return;
  }

  /// @todo
}

void cg_gen_program(const AstNode *root) {
  if (!root || root->type != AST_PROGRAM) {
    return;
  }

  // Generate head of program and internal functions
  cg_gen_head();

  // Generate all functions defined in program including @main
  for (size_t i = 0; i < root->as.program.count; i++) {
    cg_gen_func_def(root->as.program.functions[i]);
  }
}

/// @author xsirmat00 from here
/// -----------------------------------------------