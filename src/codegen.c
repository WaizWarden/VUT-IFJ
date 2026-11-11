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
    cg_gen_vars_prepass(node->as.ifStatement.condition);
    break;

  case AST_WHILE:
    cg_gen_vars_prepass(node->as.whileStatement.bodyBlock);
    cg_gen_vars_prepass(node->as.whileStatement.cond);
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
    const AstNode *expr =
        (block->type == AST_EXPR_STATEMENT)
            ? block->as.exprStatement.expr // get internal expression
            : block;
    if (isLastInFunc) {
      cg_gen_expr(expr);
      cg_adjust_return_value(expr->evalType, funcRetType);
    } else {
      cg_gen_statement(block, funcRetType);
    }
    return;
  }

  if (block->as.block.count == 0) { // do nothing if not last
    if (isLastInFunc) {
      puts("PUSHS nil@nil");
    }
    return;
  }

  // go through all commands except last
  for (size_t i = 0; i < block->as.block.count - 1; i++) {
    cg_gen_statement(block->as.block.stmts[i], funcRetType);
  }

  const AstNode *last = block->as.block.stmts[block->as.block.count - 1];

  if (!isLastInFunc) {
    cg_gen_statement(last, funcRetType);
    return;
  }

  // end of a function body
  switch (last->type) {
  case AST_EXPR_STATEMENT:
    cg_gen_expr(last->as.exprStatement.expr);
    cg_adjust_return_value(last->as.exprStatement.expr->evalType, funcRetType);
    break;

  case AST_IF:
    cg_gen_if(last, true, funcRetType);
    break;

  case AST_VAR_DECL:
  case AST_ASSIGN:
  case AST_WHILE: // retunr nil@nil
    cg_gen_statement(last, funcRetType);
    puts("PUSHS nil@nil");
    break;

  case AST_RETURN:
    cg_gen_statement(last, funcRetType);
    break;

  default:
    cg_gen_expr(last);
    cg_adjust_return_value(last->evalType, funcRetType);
    break;
  }
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

void cg_gen_expr(const AstNode *expr) {
  if (!expr) {
    return;
  }

  switch (expr->type) {
  case AST_LITERAL_INT:
    fputs("PUSHS ", stdout);
    cg_print_int(expr->as.intValue);
    putchar('\n');
    break;

  case AST_LITERAL_DOUBLE:
    fputs("PUSHS ", stdout);
    cg_print_double(expr->as.doubleValue);
    putchar('\n');
    break;

  case AST_LITERAL_STRING:
    fputs("PUSHS ", stdout);
    cg_print_string(expr->as.stringValue);
    putchar('\n');
    break;

  case AST_LITERAL_BOOL:
    fputs("PUSHS ", stdout);
    cg_print_bool(expr->as.boolValue);
    putchar('\n');
    break;

  case AST_LITERAL_UNIT:
    puts("PUSHS nil@nil");
    break;

  case AST_IDENTIFIER:
    fputs("PUSHS ", stdout);
    cg_print_var(expr->as.identif, expr->uid);
    putchar('\n');
    break;

  case AST_UNARY:
    // OP_UNARY_MINUS (IFJcode26 lacks NEGS, so compute 0 - expr)
    if (expr->evalType == T_DOUBLE) {
      puts("PUSHS float@0x0p+0");
    } else {
      puts("PUSHS int@0");
    }
    cg_gen_expr(expr->as.unary.operand);
    puts("SUBS");
    break;

  case AST_BINARY: {
    DataType leftType = expr->as.binary.left->evalType;
    DataType rightType = expr->as.binary.right->evalType;

    // 1. Evaluate left operand
    cg_gen_expr(expr->as.binary.left);

    // If expression operates on T_DOUBLE but left operand is T_INT, convert immediately on stack top
    if (expr->evalType == T_DOUBLE && leftType == T_INT) {
      puts("INT2FLOATS");
    }

    // 2. Evaluate right operand
    cg_gen_expr(expr->as.binary.right);

    // If right operand requires T_INT -> T_DOUBLE conversion
    if (expr->evalType == T_DOUBLE && rightType == T_INT) {
      puts("INT2FLOATS");
    }

    // 3. Generate operation instruction
    switch (expr->as.binary.operation) {
    case OP_ADD:
      if (leftType == T_STRING || rightType == T_STRING) {
        // String concatenation via GF@$tmp1 and GF@$tmp2
        puts("POPS GF@$tmp2\n"
             "POPS GF@$tmp1\n"
             "CONCAT GF@$tmp1 GF@$tmp1 GF@$tmp2\n"
             "PUSHS GF@$tmp1");
      } else {
        puts("ADDS");
      }
      break;

    case OP_SUB:
      puts("SUBS");
      break;

    case OP_MUL:
      puts("MULS");
      break;

    case OP_DIV:
      if (expr->evalType == T_DOUBLE) {
        puts("DIVS");
      } else {
        puts("IDIVS");
      }
      break;

    case OP_EQ:
      puts("EQS");
      break;

    case OP_NEQ:
      puts("EQS\nNOTS");
      break;

    case OP_LT:
      puts("LTS");
      break;

    case OP_LE:
      puts("GTS\nNOTS");
      break;

    case OP_GT:
      puts("GTS");
      break;

    case OP_GE:
      puts("LTS\nNOTS");
      break;

    default:
      break;
    }
    break;
  }

  case AST_FUNC_CALL:
    cg_gen_call(expr);
    break;

  default:
    break;
  }
}

void cg_gen_call(const AstNode *callNode) {
  if (!callNode || callNode->type != AST_FUNC_CALL) {
    return;
  }

  const char *name = callNode->as.funcCall.funcName;
  size_t argCount = callNode->as.funcCall.argCount;
  AstNode **args = callNode->as.funcCall.args;

  // Evaluate arguments from left to right (pushed onto data stack)
  for (size_t i = 0; i < argCount; i++) {
    cg_gen_expr(args[i]);
  }

  // --- Inline built-in functions ---
  if (strcmp(name, "print") == 0) {
    for (size_t i = 0; i < argCount; i++) {
      puts("POPS GF@$tmp1\n"
           "WRITE GF@$tmp1");
    }
    // print returns Unit (nil@nil on stack top)
    puts("PUSHS nil@nil");
    return;
  }

  if (strcmp(name, "toStr") == 0) {
    puts("POPS GF@$tmp1\n"
         "PUSHS GF@$tmp1");
    return;
  }

  if (strcmp(name, "toInt") == 0) {
    puts("POPS GF@$tmp1\n"
         "FLOAT2INT GF@$tmp1 GF@$tmp1\n"
         "PUSHS GF@$tmp1");
    return;
  }

  if (strcmp(name, "chr") == 0) {
    puts("POPS GF@$tmp1\n"
         "INT2CHAR GF@$tmp1 GF@$tmp1\n"
         "PUSHS GF@$tmp1");
    return;
  }

  if (strcmp(name, "strLength") == 0) {
    puts("POPS GF@$tmp1\n"
         "STRLEN GF@$tmp1 GF@$tmp1\n"
         "PUSHS GF@$tmp1");
    return;
  }

  if (strcmp(name, "readLine") == 0) {
    puts("READ GF@$tmp1 string\n"
         "PUSHS GF@$tmp1");
    return;
  }

  // --- Subroutine built-in functions ---
  if (strcmp(name, "readInt") == 0) {
    puts("CALL $builtin_readInt");
    return;
  }

  if (strcmp(name, "readDouble") == 0) {
    puts("CALL $builtin_readDouble");
    return;
  }

  if (strcmp(name, "ord") == 0) {
    puts("CALL $builtin_ord");
    return;
  }

  if (strcmp(name, "substr") == 0) {
    puts("CALL $builtin_substr");
    return;
  }

  // --- User defined functions ---
  printf("CALL $func_%s\n", name);
}

void cg_gen_if(const AstNode *ifNode, bool isExprOrReturn,
               DataType funcRetType) {
  if (!ifNode || ifNode->type != AST_IF) {
    return;
  }

  uint32_t id = cg_next_label_id();

  // 1. Evaluate condition
  cg_gen_expr(ifNode->as.ifStatement.condition);

  // 2. Conditional jump to else branch on false
  puts("PUSHS bool@false");
  printf("JUMPIFEQS $if_else_%" PRIu32 "\n", id);

  // 3. Generate 'then' block
  cg_gen_block(ifNode->as.ifStatement.thenBlock, isExprOrReturn, funcRetType);
  printf("JUMP $if_end_%" PRIu32 "\n", id);

  // 4. Generate 'else' block
  printf("LABEL $if_else_%" PRIu32 "\n", id);
  if (ifNode->as.ifStatement.elseBlock) {
    cg_gen_block(ifNode->as.ifStatement.elseBlock, isExprOrReturn, funcRetType);
  } else if (isExprOrReturn) {
    puts("PUSHS nil@nil");
  }

  // 5. End label
  printf("LABEL $if_end_%" PRIu32 "\n", id);
}

void cg_gen_while(const AstNode *whileNode, DataType funcRetType) {
  if (!whileNode || whileNode->type != AST_WHILE) {
    return;
  }

  uint32_t id = cg_next_label_id();

  // 1. Loop start label
  printf("LABEL $while_start_%" PRIu32 "\n", id);

  // 2. Evaluate condition
  cg_gen_expr(whileNode->as.whileStatement.cond);

  // 3. Conditional jump on condition failure (false)
  puts("PUSHS bool@false");
  printf("JUMPIFEQS $while_end_%" PRIu32 "\n", id);

  // 4. Loop body (values from loop body are popped, isLastInFunc = false)
  cg_gen_block(whileNode->as.whileStatement.bodyBlock, false, funcRetType);

  // 5. Jump to start and end label
  printf("JUMP $while_start_%" PRIu32 "\n"
         "LABEL $while_end_%" PRIu32 "\n",
         id, id);
}