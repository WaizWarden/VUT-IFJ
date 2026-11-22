/**
 * @file    codegen.c
 * @author  xoramst00
 * @brief   Generator implementation of final code IFJcode26
 *
 * @important Code generation uses two passes:
 *      1. The prepass discovers every DEFVAR in a function.
 *      2. The generation pass prints executable instructions to stdout.
 * This avoids allocating a dynamic buffer merely to insert declarations.
 *
 * @important Compiler names are kept separate from source-level names:
 *      - GF@$tmp1 ... GF@$tmp3 are temporary global values.
 *      - $func_... and $builtin_... identify functions.
 *      - $if_..., $while_..., and $for_... identify control-flow labels.
 *      - LF@name$uid stores a local variable with its semantic UID.
 *
 * @important Calling convention:
 *      - Arguments are pushed onto the stack before CALL.
 *      - A function pops them in reverse order, with the last argument topmost.
 *      - A return value remains on the stack, including nil@nil for Unit.
 *
 * @important Built-in function handling:
 *  inline:
 *      readLine(): string
 *      toInt(val: Double): Int
 *      strLength(s: String): Int
 *      chr(code: Int): String
 *      toStr(value: Any): String
 *      print(value: Any): Unit
 *      floor(val: Double): Double
 *  extra func:
 *      readInt(): Int
 *      readDouble(): Double
 *      ord(s: String, i: Int): Int
 *      substr(s, start, end)
 *
 * @note AI use: static code analysis, semantic comment fixes and
 * standardization, structuring code prompt: "standardize comment format and
 * describe any semantic comment inacuracies" workflow: If I understand the
 * issue, I implement what I can, and when I don't understand, I ask gemini or
 * other model to explain the issue and help me with implementation.
 *
 */

// characters at or below this ASCII value must be escaped in string literals
#define SPECIAL_CHAR_IN_STRING_MAX 32

#include "codegen.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

// labels receive a module-wide sequence number so each generated jump is unique
static uint32_t label_counter = 0;

// return the next label ID
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

    // escape characters are already parsed into a single character
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

  // start at $main after the interpreter loads the generated program
  puts(".IFJcode26\n"
       "DEFVAR GF@$tmp1\n"
       "DEFVAR GF@$tmp2\n"
       "DEFVAR GF@$tmp3\n"
       "CALL $main\n"
       "EXIT int@0\n");

  // reading primitives share small helper labels instead of being repeated

  // nil and malformed input become the defaults expected by IFJ26
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

  // ord(s, i) pops both arguments and leaves the character code on the stack
  // an invalid index reports -1
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

  // subStr(s, start, end) builds the substring in a private local frame
  puts("LABEL $builtin_substr\n"

       // create a fresh frame so helper variables do not collide with callers
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

       // reject negative, backwards, or out-of-bounds indexes
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

       // append one character at a time until start reaches end
       "MOVE LF@res string@\n"
       "LABEL $substr_loop\n"
       "JUMPIFEQ $substr_done LF@start LF@end\n"
       "GETCHAR LF@ch LF@str LF@start\n"
       "CONCAT LF@res LF@res LF@ch\n"
       "ADD LF@start LF@start int@1\n"
       "JUMP $substr_loop\n"

       // restore the caller frame and leave the resulting string on the stack
       "LABEL $substr_done\n"
       "PUSHS LF@res\n"
       "POPFRAME\n"
       "RETURN\n"
       "LABEL $substr_err40\n"
       "EXIT int@40\n");
}

/**
 * @brief Discovers and emits every local variable declaration in an AST
 * subtree.
 *
 * This recursive pass runs before executable instructions are generated. It
 * makes declarations visible throughout a function, including variables located
 * in nested blocks.
 *
 * @param node AST subtree to scan.
 */
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

  case AST_EXPR_STATEMENT:
    cg_gen_vars_prepass(node->as.exprStatement.expr);
    break;

  case AST_BINARY:
    cg_gen_vars_prepass(node->as.binary.left);
    cg_gen_vars_prepass(node->as.binary.right);
    break;

  case AST_UNARY:
    cg_gen_vars_prepass(node->as.unary.operand);
    break;

  case AST_FUNC_CALL:
    for (size_t i = 0; i < node->as.funcCall.argCount; i++) {
      cg_gen_vars_prepass(node->as.funcCall.args[i]);
    }
    break;

  case AST_FOR:
    fputs("DEFVAR ", stdout);
    cg_print_var(node->as.forStatement.varName, node->as.forStatement.varUid);
    putchar('\n');
    printf("DEFVAR LF@%%end$%u\n"   // value to end on
           "DEFVAR LF@%%step$%u\n"  // value to step by
           "DEFVAR LF@%%cond$%u\n", // current value to check condition with
           node->as.forStatement.varUid, node->as.forStatement.varUid,
           node->as.forStatement.varUid);
    cg_gen_vars_prepass(node->as.forStatement.from);
    cg_gen_vars_prepass(node->as.forStatement.to);
    cg_gen_vars_prepass(node->as.forStatement.step);
    for (size_t i = 0; i < node->as.forStatement.guardCount; i++) {
      cg_gen_vars_prepass(node->as.forStatement.guards[i]);
    }
    cg_gen_vars_prepass(node->as.forStatement.body);
    break;

  default: // literals and identifiers do not need declarations
    break;
  }
}

/**
 * @brief Generates a function label, frame setup, parameters, and function
 * body.
 * @param node AST node of type AST_FUNC_DEF.
 */
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

  // parameters are ordinary local variables in this function frame
  for (uint16_t i = 0; i < node->as.funcDef.paramCount; i++) {
    fputs("DEFVAR ", stdout);
    cg_print_var(node->as.funcDef.params[i].name,
                 node->as.funcDef.params[i].uid);
    putchar('\n');
  }

  // arguments arrive in reverse order, so pop them back in signature order
  for (int i = (int)node->as.funcDef.paramCount - 1; i >= 0; i--) {
    fputs("POPS ", stdout);
    cg_print_var(node->as.funcDef.params[i].name,
                 node->as.funcDef.params[i].uid);
    putchar('\n');
  }

  // discover declarations before generating executable instructions
  cg_gen_vars_prepass(node->as.funcDef.body);

  // generate function body
  cg_gen_block(node->as.funcDef.body, true, node->as.funcDef.retType);

  puts("POPFRAME\n"
       "RETURN");
}

/**
 * @brief Applies implicit return conversions and the Unit return convention.
 * @param exprType Type of the expression being returned.
 * @param funcRetType Declared return type of the enclosing function.
 */
static void cg_adjust_return_value(DataType exprType, DataType funcRetType) {

  if (funcRetType == T_UNIT) {
    // unit has no payload, so always normalize it to nil@nil
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

  case AST_VAR_DECL:
    if (stmt->as.varDecl.initExpr) {
      cg_gen_expr(stmt->as.varDecl.initExpr);

      if (stmt->as.varDecl.hasDeclaredType &&
          stmt->as.varDecl.declaredType == T_DOUBLE &&
          stmt->as.varDecl.initExpr->evalType == T_INT) {
        puts("INT2FLOATS");
      }
    } else {
      // an omitted initializer is represented by nil
      puts("PUSHS nil@nil");
    }

    // defvar for this variable was emitted by the prepass
    fputs("POPS ", stdout);
    cg_print_var(stmt->as.varDecl.name, stmt->uid);
    putchar('\n');
    break;

  case AST_ASSIGN:
    cg_gen_expr(stmt->as.assign.value);

    // assignment also allows the sole implicit conversion, Int to Double
    if (stmt->evalType == T_DOUBLE &&
        stmt->as.assign.value->evalType == T_INT) {
      puts("INT2FLOATS");
    }

    // uid keeps shadowed variables in nested blocks from colliding
    fputs("POPS ", stdout);
    cg_print_var(stmt->as.assign.name, stmt->uid);
    putchar('\n');
    break;

  case AST_RETURN:

    if (stmt->as.ret.expr == NULL) {
      puts("PUSHS nil@nil");
    } else {
      cg_gen_expr(stmt->as.ret.expr);
      cg_adjust_return_value(stmt->as.ret.expr->evalType, funcRetType);
    }

    puts("POPFRAME\n"
         "RETURN");
    break;

  case AST_IF:
    cg_gen_if(stmt, false, funcRetType);
    break;

  case AST_WHILE:
    cg_gen_while(stmt, funcRetType);
    break;

  case AST_EXPR_STATEMENT:
    cg_gen_expr(stmt->as.exprStatement.expr);
    puts("POPS GF@$tmp1"); // the result is intentionally discarded
    break;

  case AST_FUNC_CALL:
    cg_gen_call(stmt);
    puts("POPS GF@$tmp1"); // calls used as statements discard their value
    break;

  case AST_FOR:
    cg_gen_for(stmt, funcRetType);
    break;

  default:
    cg_gen_expr(stmt);
    puts("POPS GF@$tmp1"); // this is not the function tail
    break;
  }
}

/**
 * @brief Generates the final statement or expression of a function body.
 *
 * The final item is special because it must leave a correctly typed result
 * on the stack instead of merely discarding the result of a statement.
 *
 * @param node AST node that occupies the tail position.
 * @param funcRetType Declared return type of the enclosing function.
 */
static void cg_gen_tail(const AstNode *node, DataType funcRetType) {
  if (!node) {
    return;
  }

  switch (node->type) {
  case AST_EXPR_STATEMENT:
    cg_gen_expr(node->as.exprStatement.expr);
    cg_adjust_return_value(node->as.exprStatement.expr->evalType, funcRetType);
    break;

  case AST_IF:
    cg_gen_if(node, true, funcRetType);
    break;

  case AST_VAR_DECL:
  case AST_ASSIGN:
  case AST_WHILE:
  case AST_FOR:
    cg_gen_statement(node, funcRetType);
    puts("PUSHS nil@nil");
    break;

  case AST_RETURN:
    cg_gen_statement(node, funcRetType);
    break;

  default:
    cg_gen_expr(node);
    cg_adjust_return_value(node->evalType, funcRetType);
    break;
  }
}

void cg_gen_block(const AstNode *block, bool isLastInFunc,
                  DataType funcRetType) {
  if (!block) {
    return;
  }

  if (block->type != AST_BLOCK) {
    if (isLastInFunc) {
      cg_gen_tail(block, funcRetType);
    } else {
      cg_gen_statement(block, funcRetType);
    }
    return;
  }

  if (block->as.block.count == 0) {
    if (isLastInFunc) {
      puts("PUSHS nil@nil");
    }
    return;
  }

  // statements before the tail execute normally and discard no visible result
  for (size_t i = 0; i < block->as.block.count - 1; i++) {
    cg_gen_statement(block->as.block.stmts[i], funcRetType);
  }

  const AstNode *last = block->as.block.stmts[block->as.block.count - 1];

  if (!isLastInFunc) {
    cg_gen_statement(last, funcRetType);
    return;
  }

  // the last item decides what the function leaves on the stack
  cg_gen_tail(last, funcRetType);
}

void cg_gen_program(const AstNode *root) {
  if (!root || root->type != AST_PROGRAM) {
    return;
  }

  // generate the head and reusable built-in functions
  cg_gen_head();

  // generate every source function, including $main
  for (size_t i = 0; i < root->as.program.count; i++) {
    cg_gen_func_def(root->as.program.functions[i]);
  }
}

/**
 * @brief Generates IFJcode26 instructions for an expression and pushes result
 * onto data stack
 *
 * @param expr AST node representing the expression
 */
void cg_gen_expr(const AstNode *expr) {
  if (!expr) {
    return;
  }

  switch (expr->type) {
  case AST_LITERAL_INT:
    fputs("PUSHS ", stdout);
    cg_print_int((int64_t)expr->as.intValue);
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
    fputs("PUSHS ", stdout);
    cg_print_unit();
    putchar('\n');
    break;

  case AST_IDENTIFIER:
    fputs("PUSHS ", stdout);
    cg_print_var(expr->as.identif, expr->uid);
    putchar('\n');
    break;

  case AST_FUNC_CALL:
    cg_gen_call(expr);
    break;

  case AST_IF:
    cg_gen_if(expr, true, expr->evalType);
    break;

  case AST_UNARY: {
    if (expr->as.unary.op == OP_UNARY_MINUS) {
      if (expr->evalType == T_INT) {
        puts("PUSHS int@0");
      } else {
        puts("PUSHS float@0x0p+0");
      }
      cg_gen_expr(expr->as.unary.operand);
      puts("SUBS");
    } else if (expr->as.unary.op == OP_UNARY_NOT) {
      cg_gen_expr(expr->as.unary.operand);
      puts("NOTS");
    }
    break;
  }

  case AST_BINARY: {
    const AstNode *left = expr->as.binary.left;
    const AstNode *right = expr->as.binary.right;
    BinaryOperation operation = expr->as.binary.operation;

    // && and || evaluate left to right and skip the right operand when
    // the left operand already determines the result
    if (operation == OP_AND) {
      uint32_t labelId = cg_next_label_id();
      cg_gen_expr(left);
      printf("PUSHS bool@false\n"
             "JUMPIFEQS $and_false_%u\n",
             labelId);
      cg_gen_expr(right);
      printf("JUMP $and_end_%u\n"
             "LABEL $and_false_%u\n"
             "PUSHS bool@false\n"
             "LABEL $and_end_%u\n",
             labelId, labelId, labelId);
      break;
    }

    if (operation == OP_OR) {
      uint32_t labelId = cg_next_label_id();
      cg_gen_expr(left);
      printf("PUSHS bool@true\n"
             "JUMPIFEQS $or_true_%u\n",
             labelId);
      cg_gen_expr(right);
      printf("JUMP $or_end_%u\n"
             "LABEL $or_true_%u\n"
             "PUSHS bool@true\n"
             "LABEL $or_end_%u\n",
             labelId, labelId, labelId);
      break;
    }

    // evaluate the left operand and promote Int to Double if needed
    cg_gen_expr(left);
    if (left->evalType == T_INT && right->evalType == T_DOUBLE) {
      puts("INT2FLOATS");
    }

    // evaluate the right operand and promote Int to Double if needed
    cg_gen_expr(right);
    if (right->evalType == T_INT && left->evalType == T_DOUBLE) {
      puts("INT2FLOATS");
    }

    // apply the operation to the values left on the data stack
    switch (operation) {
    case OP_ADD:
      if (expr->evalType == T_STRING) {
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
      if (expr->evalType == T_INT) {
        puts("IDIVS");
      } else {
        puts("DIVS");
      }
      break;

    case OP_LT:
      puts("LTS");
      break;

    case OP_GT:
      puts("GTS");
      break;

    case OP_EQ:
      puts("EQS");
      break;

    case OP_NEQ:
      puts("EQS\n"
           "NOTS");
      break;

    case OP_LE:
      puts("GTS\n"
           "NOTS");
      break;

    case OP_GE:
      puts("LTS\n"
           "NOTS");
      break;

    default:
      break;
    }
    break;
  }

  default:
    break;
  }
}

/**
 * @brief Generates IFJcode26 instructions for an if-else statement or
 * expression
 *
 * @param ifNode AST node of type AST_IF
 * @param isExprOrReturn True if result value must be left on the stack
 * @param funcRetType Return type of enclosing function
 */
void cg_gen_if(const AstNode *ifNode, bool isExprOrReturn,
               DataType funcRetType) {
  if (!ifNode) {
    return;
  }

  uint32_t labelId = cg_next_label_id();

  // adding false lets JUMPIFEQS replace a separate Boolean variable
  cg_gen_expr(ifNode->as.ifStatement.condition);
  printf("PUSHS bool@false\n"
         "JUMPIFEQS $if_else_%u\n",
         labelId);

  // execute the then branch, then jump past the else branch
  cg_gen_block(ifNode->as.ifStatement.thenBlock, isExprOrReturn, funcRetType);
  printf("JUMP $if_end_%u\n", labelId);

  // an expression-style if needs an explicit nil if there is no else
  printf("LABEL $if_else_%u\n", labelId);
  if (ifNode->as.ifStatement.elseBlock) {
    cg_gen_block(ifNode->as.ifStatement.elseBlock, isExprOrReturn, funcRetType);
  } else if (isExprOrReturn) {
    puts("PUSHS nil@nil");
  }

  printf("LABEL $if_end_%u\n", labelId);
}

/**
 * @brief Generates IFJcode26 instructions for a while loop
 *
 * @param whileNode AST node of type AST_WHILE
 * @param funcRetType Return type of enclosing function
 */
void cg_gen_while(const AstNode *whileNode, DataType funcRetType) {
  if (!whileNode) {
    return;
  }

  uint32_t labelId = cg_next_label_id();

  printf("LABEL $while_start_%u\n", labelId);

  // a false condition exits the while loop
  cg_gen_expr(whileNode->as.whileStatement.cond);
  printf("PUSHS bool@false\n"
         "JUMPIFEQS $while_end_%u\n",
         labelId);

  // execute the loop body
  cg_gen_block(whileNode->as.whileStatement.bodyBlock, false, funcRetType);

  printf("JUMP $while_start_%u\n"
         "LABEL $while_end_%u\n",
         labelId, labelId);
}

/**
 * @brief Generates a CONTROL for loop using variables created by the prepass.
 *
 * The start, end, and step values are evaluated once. Each iteration compares
 * the current loop variable with the end, evaluates guards, executes the body,
 * and advances by the configured step.
 *
 * @param forNode AST node of type AST_FOR.
 * @param funcRetType Return type of the enclosing function.
 */
void cg_gen_for(const AstNode *forNode, DataType funcRetType) {
  if (!forNode) {
    return;
  }

  const uint32_t varUid = forNode->as.forStatement.varUid;
  const uint32_t labelId = cg_next_label_id();
  const char *varName = forNode->as.forStatement.varName;

  // evaluate the first value once and place it into the loop variable
  cg_gen_expr(forNode->as.forStatement.from);

  printf("POPS ");
  cg_print_var(varName, varUid);
  putchar('\n');

  // save the end value once so it is not reevaluated for every iteration
  cg_gen_expr(forNode->as.forStatement.to);

  printf("POPS LF@%%end$%u\n", varUid);

  // when by is omitted, Scala-style range iteration starts with step 1
  if (forNode->as.forStatement.step) {
    cg_gen_expr(forNode->as.forStatement.step);
  } else {
    puts("PUSHS int@1");
  }

  printf("POPS LF@%%step$%u\n"
         "LABEL $for_start_%u\n",
         varUid, labelId);

  // zero is never useful for a range; it would make the loop never advance
  printf("JUMPIFEQ $for_zero_%u LF@%%step$%u int@0\n"
         "LT LF@%%cond$%u LF@%%step$%u int@0\n"
         "JUMPIFEQ $for_negative_%u LF@%%cond$%u bool@true\n",
         labelId, varUid, varUid, varUid, labelId, varUid);

  // positive steps stop when the loop variable moves past the right boundary
  if (forNode->as.forStatement.inclusive) {
    printf("GT LF@%%cond$%u ", varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n",
           varUid, labelId, varUid);
  } else {
    printf("GT LF@%%cond$%u ", varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n"
           "EQ LF@%%cond$%u ",
           varUid, labelId, varUid, varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n"
           "JUMP $for_body_%u\n",
           varUid, labelId, varUid, labelId);
  }

  printf("JUMP $for_body_%u\n"
         "LABEL $for_negative_%u\n",
         labelId, labelId);

  // negative steps stop when the loop variable moves past the left boundary
  if (forNode->as.forStatement.inclusive) {
    printf("LT LF@%%cond$%u ", varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n",
           varUid, labelId, varUid);
  } else {
    printf("LT LF@%%cond$%u ", varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n"
           "EQ LF@%%cond$%u ",
           varUid, labelId, varUid, varUid);
    cg_print_var(varName, varUid);
    printf(" LF@%%end$%u\n"
           "JUMPIFEQ $for_end_%u LF@%%cond$%u bool@true\n",
           varUid, labelId, varUid);
  }

  // guards behave like a chain of AND conditions
  // the first false guard skips the body and also skips later guards here
  printf("LABEL $for_body_%u\n", labelId);

  for (size_t i = 0; i < forNode->as.forStatement.guardCount; i++) {
    cg_gen_expr(forNode->as.forStatement.guards[i]);

    printf("PUSHS bool@false\n"
           "JUMPIFEQS $for_next_%u\n",
           labelId);
  }

  cg_gen_block(forNode->as.forStatement.body, false, funcRetType);

  // advance by the saved step, then start the next boundary check
  printf("LABEL $for_next_%u\n", labelId);

  printf("ADD LF@");
  cg_print_var(varName, varUid);
  printf(" LF@");
  cg_print_var(varName, varUid);
  printf(" LF@%%step$%u\n"
         "JUMP $for_start_%u\n"
         "LABEL $for_zero_%u\n"
         "EXIT int@40\n"
         "LABEL $for_end_%u\n",
         varUid, labelId, labelId, labelId);
}

/**
 * @brief Generates IFJcode26 instructions for built-in and user function calls
 *
 * @param callNode AST node of type AST_FUNC_CALL
 */
void cg_gen_call(const AstNode *callNode) {
  if (!callNode) {
    return;
  }

  const char *name = callNode->as.funcCall.funcName;
  size_t argCount = callNode->as.funcCall.argCount;
  AstNode **args = callNode->as.funcCall.args;

  // inline built-ins keep the result directly on the data stack

  if (strcmp(name, "readLine") == 0) {
    uint32_t labelId = cg_next_label_id();

    // EOF is represented by nil in IFJcode26, which is not a valid String
    // here, so replace it with an empty String before returning
    printf("READ GF@$tmp1 string\n"
           "JUMPIFNEQ $readLine_ok_%u GF@$tmp1 nil@nil\n"
           "MOVE GF@$tmp1 string@\n"
           "LABEL $readLine_ok_%u\n"
           "PUSHS GF@$tmp1\n",
           labelId, labelId);

    return;
  }

  if (strcmp(name, "readBoolean") == 0) {
    uint32_t labelId = cg_next_label_id();

    // readBoolean follows readInt/readDouble and treats nil as false
    // which keeps an EOF or malformed value from leaking as a nil result
    printf("READ GF@$tmp1 bool\n"
           "JUMPIFNEQ $readBool_ok_%u GF@$tmp1 nil@nil\n"
           "MOVE GF@$tmp1 bool@false\n"
           "LABEL $readBool_ok_%u\n"
           "PUSHS GF@$tmp1\n",
           labelId, labelId);

    return;
  }

  if (strcmp(name, "toInt") == 0) {
    if (argCount > 0) {
      // eval the argument first so nested VALMEM or function expressions run
      // before the conversion is applied
      cg_gen_expr(args[0]);

      if (args[0]->evalType == T_DOUBLE) {
        // the stack version truncates Double directly toward zero
        puts("FLOAT2INTS");
      }
    }

    return;
  }

  if (strcmp(name, "strLength") == 0) {
    if (argCount > 0) {
      // move the receiver out of the expression stack before STRLEN
      // because STRLEN writes into a named variable
      cg_gen_expr(args[0]);

      puts("POPS GF@$tmp1\n"
           "STRLEN GF@$tmp1 GF@$tmp1\n"
           "PUSHS GF@$tmp1");
    }

    return;
  }

  if (strcmp(name, "chr") == 0) {
    if (argCount > 0) {
      // chr needs no temporary variable because INT2CHARS is stack-based
      cg_gen_expr(args[0]);

      puts("INT2CHARS");
    }

    return;
  }

  if (strcmp(name, "toStr") == 0) {
    if (argCount > 0) {
      // leave the original value on the stack and replace it below with text
      // using the conversion appropriate for the statically known type
      cg_gen_expr(args[0]);

      DataType argType = args[0]->evalType;

      if (argType == T_INT) {
        // INT2STR is the codegen counterpart of toStr for integers
        puts("POPS GF@$tmp1\n"
             "INT2STR GF@$tmp1 GF@$tmp1\n"
             "PUSHS GF@$tmp1");
      } else if (argType == T_DOUBLE) {
        // FLOAT2STR also provides the IFJ26 required two-decimal format
        puts("POPS GF@$tmp1\n"
             "FLOAT2STR GF@$tmp1 GF@$tmp1\n"
             "PUSHS GF@$tmp1");
      } else if (argType == T_BOOLEAN) {
        uint32_t labelId = cg_next_label_id();

        // IFJcode26 has no direct boolToStr conversion
        // push the result of a small comparison and choose the matching text
        printf("PUSHS bool@true\n"
               "JUMPIFNEQS $tostr_false_%u\n"
               "PUSHS string@true\n"
               "JUMP $tostr_end_%u\n"
               "LABEL $tostr_false_%u\n"
               "PUSHS string@false\n"
               "LABEL $tostr_end_%u\n",
               labelId, labelId, labelId, labelId);
      } else if (argType == T_UNIT) {
        // Unit has no payload, so its textual representation is fixed
        puts("POPS GF@$tmp1\n"
             "PUSHS string@()");
      } else if (argType == T_STRING) {
        // String is already text, so pop and push it without conversion
        puts("POPS GF@$tmp1\n"
             "PUSHS GF@$tmp1");
      }
    }

    return;
  }

  if (strcmp(name, "floor") == 0) {
    if (argCount > 0) {
      // floor is implemented from existing primitive conversions
      // no dedicated FLOOR instruction is available in IFJcode26
      cg_gen_expr(args[0]);

      uint32_t labelId = cg_next_label_id();

      // truncate toward zero, convert back to Double, and lower negative
      // fractional values by one after the original check
      printf("POPS GF@$tmp1\n"
             "FLOAT2INT GF@$tmp2 GF@$tmp1\n"
             "INT2FLOAT GF@$tmp3 GF@$tmp2\n"
             "LT GF@$tmp4 GF@$tmp1 GF@$tmp3\n"
             "JUMPIFEQ $floor_end_%u GF@$tmp4 bool@false\n"
             "SUB GF@$tmp3 GF@$tmp3 float@0x1p+0\n"
             "LABEL $floor_end_%u\n"
             "PUSHS GF@$tmp3\n",
             labelId, labelId);
    }

    return;
  }

  if (strcmp(name, "print") == 0) {
    if (argCount > 0) {
      // Unit is the only value that WRITE cannot print directly as requested
      cg_gen_expr(args[0]);

      if (args[0]->evalType == T_UNIT) {
        puts("POPS GF@$tmp1\n"
             "WRITE string@()\n"
             "PUSHS nil@nil");
      } else {
        puts("POPS GF@$tmp1\n"
             "WRITE GF@$tmp1\n"
             "PUSHS nil@nil");
      }
    } else {
      puts("PUSHS nil@nil");
    }

    return;
  }

  // extra built-ins are implemented as reusable labels

  if (strcmp(name, "readInt") == 0) {
    puts("CALL $builtin_readInt");

    return;
  }

  if (strcmp(name, "readDouble") == 0) {
    puts("CALL $builtin_readDouble");

    return;
  }

  if (strcmp(name, "ord") == 0) {
    for (size_t idx = 0; idx < argCount; idx++) {
      cg_gen_expr(args[idx]);
    }

    puts("CALL $builtin_ord");

    return;
  }

  if (strcmp(name, "subStr") == 0) {
    for (size_t i = 0; i < argCount; i++) {
      cg_gen_expr(args[i]);
    }

    puts("CALL $builtin_substr");

    return;
  }

  // all remaining calls are assumed to refer to source-level functions
  for (size_t idx = 0; idx < argCount; idx++) {
    cg_gen_expr(args[idx]);
  }

  printf("CALL $func_%s\n", name);
}
