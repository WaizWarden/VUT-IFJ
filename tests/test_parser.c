/**
 * @file    test_parser.c
 * @author  xhorakj01
 * @brief   Tests for parser analysis of expressions
 *
 * Each expression is fed to the scanner from a string via fmemopen() and the
 * shape of the resulting AST is checked with asserts.
 */

#include "test_parser.h"
#include "../src/parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Helper function that takes string and runs the parser_parse
 *
 * @retrun AstNode*
 */
static AstNode *parser_program_helper(const char *source){
    FILE *file = fmemopen((void *)source, strlen(source), "r");
    assert(file != NULL);

    AstNode *program = parser_parse(file);

    fclose(file);
    scanner_destroy();
    return program;
}

/**
 * @brief Tests the validity of prolog parsing ("import ifj26.*"")
 */
static void test_prolog_validity(void){
    AstNode *program = parser_program_helper(
             "import ifj26.*\n"
             "def answer(): Int = 42\n");

    assert(program != NULL);
    assert(program->type == AST_PROGRAM);
    assert(program->as.program.count == 1);

    const AstNode *function = program->as.program.functions[0];
    assert(function != NULL);
    assert(function->type == AST_FUNC_DEF);
    assert(strcmp(function->as.funcDef.name, "answer") == 0);
    assert(!function->as.funcDef.isMain);
    assert(function->as.funcDef.paramCount == 0);
    assert(function->as.funcDef.retType == T_INT);

    const AstNode *body = function->as.funcDef.body;
    assert(body != NULL);
    assert(body->type == AST_LITERAL_INT);
    assert(body->as.intValue == 42);

    ast_free(program);
}
/**
 * @brief Testing whenether the parser parser parameters of function correctly
 */
static void test_function_parameters(void) {
    AstNode *program = parser_program_helper(
        "import ifj26.*\n"
        "def choose(number: Int, text: String): Int = number\n");

    assert(program != NULL);
    assert(program->type == AST_PROGRAM);
    assert(program->as.program.count == 1);

    const AstNode *function = program->as.program.functions[0];
    assert(function != NULL);
    assert(function->type == AST_FUNC_DEF);
    assert(strcmp(function->as.funcDef.name, "choose") == 0);
    assert(function->as.funcDef.retType == T_INT);
    assert(function->as.funcDef.paramCount == 2);
    assert(function->as.funcDef.params != NULL);

    const Parameter *params = function->as.funcDef.params;
    assert(strcmp(params[0].name, "number") == 0);
    assert(params[0].type == T_INT);
    assert(strcmp(params[1].name, "text") == 0);
    assert(params[1].type == T_STRING);

    const AstNode *body = function->as.funcDef.body;
    assert(body != NULL);
    assert(body->type == AST_IDENTIFIER);
    assert(strcmp(body->as.identif, "number") == 0);

    ast_free(program);
  }

/**
 * @brief Tests main anotation
 */
static void test_main_annotation(void) {
    const char *sources[] = {
        "import ifj26.*\n"
        "@main def runFoo(): Unit = ()\n",

        "import ifj26.*\n"
        "@main\n"
        "def runFoo(): Unit = ()\n",
    };

    for (size_t index = 0; index < sizeof(sources) / sizeof(sources[0]);
        index++) {
        AstNode *program = parser_program_helper(sources[index]);

        assert(program != NULL);
        assert(program->type == AST_PROGRAM);
        assert(program->as.program.count == 1);

        const AstNode *function = program->as.program.functions[0];
        assert(function != NULL);
        assert(function->type == AST_FUNC_DEF);
        assert(function->as.funcDef.isMain);
        assert(strcmp(function->as.funcDef.name, "runFoo") == 0);
        assert(function->as.funcDef.paramCount == 0);
        assert(function->as.funcDef.retType == T_UNIT);

        const AstNode *body = function->as.funcDef.body;
        assert(body != NULL);
        assert(body->type == AST_LITERAL_UNIT);

        ast_free(program);
    }
}

/**
 * @brief Tests the indentation of block
 */
static void test_block_statements(void) {
     AstNode *program = parser_program_helper(
         "import ifj26.*\n"
         "def calculate(): Int =\n"
         "    var x: Int = 5\n"
         "    x = 6\n"
         "    return x\n");

     assert(program != NULL);
     assert(program->type == AST_PROGRAM);
     assert(program->as.program.count == 1);

     const AstNode *function = program->as.program.functions[0];
     assert(function != NULL);
     assert(function->type == AST_FUNC_DEF);

     const AstNode *body = function->as.funcDef.body;
     assert(body != NULL);
     assert(body->type == AST_BLOCK);
     assert(body->as.block.count == 3);

     const AstNode *declaration = body->as.block.stmts[0];
     assert(declaration != NULL);
     assert(declaration->type == AST_VAR_DECL);
     assert(strcmp(declaration->as.varDecl.name, "x") == 0);
     assert(declaration->as.varDecl.isModifiable);
     assert(declaration->as.varDecl.hasDeclaredType);
     assert(declaration->as.varDecl.declaredType == T_INT);
     assert(declaration->as.varDecl.initExpr != NULL);
     assert(declaration->as.varDecl.initExpr->type == AST_LITERAL_INT);
     assert(declaration->as.varDecl.initExpr->as.intValue == 5);

     const AstNode *assignment = body->as.block.stmts[1];
     assert(assignment != NULL);
     assert(assignment->type == AST_ASSIGN);
     assert(strcmp(assignment->as.assign.name, "x") == 0);
     assert(assignment->as.assign.value != NULL);
     assert(assignment->as.assign.value->type == AST_LITERAL_INT);
     assert(assignment->as.assign.value->as.intValue == 6);

     const AstNode *returnNode = body->as.block.stmts[2];
     assert(returnNode != NULL);
     assert(returnNode->type == AST_RETURN);
     assert(returnNode->as.ret.expr != NULL);
     assert(returnNode->as.ret.expr->type == AST_IDENTIFIER);
     assert(strcmp(returnNode->as.ret.expr->as.identif, "x") == 0);

     ast_free(program);
   }

/**
 * @brief Tests the ';' and an expression that begins with an identifier into the precedence mechanism
 */
static void test_semicolon_statements(void) {
     AstNode *program = parser_program_helper(
         "import ifj26.*\n"
         "def increment(): Int =\n"
         "    var x = 5; x = x + 1\n"
         "    return x\n");

     assert(program != NULL);
     assert(program->type == AST_PROGRAM);
     assert(program->as.program.count == 1);

     const AstNode *function = program->as.program.functions[0];
     assert(function != NULL);
     assert(function->type == AST_FUNC_DEF);

     const AstNode *body = function->as.funcDef.body;
     assert(body != NULL);
     assert(body->type == AST_BLOCK);
     assert(body->as.block.count == 3);

     const AstNode *declaration = body->as.block.stmts[0];
     assert(declaration != NULL);
     assert(declaration->type == AST_VAR_DECL);
     assert(strcmp(declaration->as.varDecl.name, "x") == 0);
     assert(declaration->as.varDecl.isModifiable);
     assert(!declaration->as.varDecl.hasDeclaredType);
     assert(declaration->as.varDecl.initExpr != NULL);
     assert(declaration->as.varDecl.initExpr->type == AST_LITERAL_INT);
     assert(declaration->as.varDecl.initExpr->as.intValue == 5);

     const AstNode *assignment = body->as.block.stmts[1];
     assert(assignment != NULL);
     assert(assignment->type == AST_ASSIGN);
     assert(strcmp(assignment->as.assign.name, "x") == 0);

     const AstNode *expression = assignment->as.assign.value;
     assert(expression != NULL);
     assert(expression->type == AST_BINARY);
     assert(expression->as.binary.operation == OP_ADD);

     const AstNode *left = expression->as.binary.left;
     const AstNode *right = expression->as.binary.right;
     assert(left != NULL);
     assert(left->type == AST_IDENTIFIER);
     assert(strcmp(left->as.identif, "x") == 0);
     assert(right != NULL);
     assert(right->type == AST_LITERAL_INT);
     assert(right->as.intValue == 1);

     const AstNode *returnNode = body->as.block.stmts[2];
     assert(returnNode != NULL);
     assert(returnNode->type == AST_RETURN);
     assert(returnNode->as.ret.expr != NULL);
     assert(returnNode->as.ret.expr->type == AST_IDENTIFIER);
     assert(strcmp(returnNode->as.ret.expr->as.identif, "x") == 0);

     ast_free(program);
   }
/**
 * @brief testing function call as a standalone statement
 */
static void test_call_statement(void) {
     AstNode *program = parser_program_helper(
         "import ifj26.*\n"
         "def demo(value: Double): Unit =\n"
         "    output(7, \"hello\", value)\n");

     assert(program != NULL);
     assert(program->type == AST_PROGRAM);
     assert(program->as.program.count == 1);

     const AstNode *function = program->as.program.functions[0];
     assert(function != NULL);
     assert(function->type == AST_FUNC_DEF);

     const AstNode *body = function->as.funcDef.body;
     assert(body != NULL);
     assert(body->type == AST_BLOCK);
     assert(body->as.block.count == 1);

     const AstNode *statement = body->as.block.stmts[0];
     assert(statement != NULL);
     assert(statement->type == AST_EXPR_STATEMENT);

     const AstNode *call = statement->as.exprStatement.expr;
     assert(call != NULL);
     assert(call->type == AST_FUNC_CALL);
     assert(strcmp(call->as.funcCall.funcName, "output") == 0);
     assert(call->as.funcCall.argCount == 3);
     assert(call->as.funcCall.args != NULL);

     const AstNode *first = call->as.funcCall.args[0];
     assert(first != NULL);
     assert(first->type == AST_LITERAL_INT);
     assert(first->as.intValue == 7);

     const AstNode *second = call->as.funcCall.args[1];
     assert(second != NULL);
     assert(second->type == AST_LITERAL_STRING);
     assert(strcmp(second->as.stringValue, "hello") == 0);

     const AstNode *third = call->as.funcCall.args[2];
     assert(third != NULL);
     assert(third->type == AST_IDENTIFIER);
     assert(strcmp(third->as.identif, "value") == 0);

     ast_free(program);
   }

/**
 * @brief Testing if, else
 */
static void test_if_else(void) {
     AstNode *program = parser_program_helper(
         "import ifj26.*\n"
         "def choose(x: Int): Int =\n"
         "    if x > 0 then\n"
         "        return 1\n"
         "    else\n"
         "        return 0\n");

     assert(program != NULL);
     assert(program->type == AST_PROGRAM);
     assert(program->as.program.count == 1);

     const AstNode *function = program->as.program.functions[0];
     assert(function != NULL);
     assert(function->type == AST_FUNC_DEF);

     const AstNode *body = function->as.funcDef.body;
     assert(body != NULL);
     assert(body->type == AST_BLOCK);
     assert(body->as.block.count == 1);

     const AstNode *ifNode = body->as.block.stmts[0];
     assert(ifNode != NULL);
     assert(ifNode->type == AST_IF);

     const AstNode *condition = ifNode->as.ifStatement.condition;
     assert(condition != NULL);
     assert(condition->type == AST_BINARY);
     assert(condition->as.binary.operation == OP_GT);

     const AstNode *left = condition->as.binary.left;
     const AstNode *right = condition->as.binary.right;
     assert(left != NULL);
     assert(left->type == AST_IDENTIFIER);
     assert(strcmp(left->as.identif, "x") == 0);
     assert(right != NULL);
     assert(right->type == AST_LITERAL_INT);
     assert(right->as.intValue == 0);

     const AstNode *thenBlock = ifNode->as.ifStatement.thenBlock;
     assert(thenBlock != NULL);
     assert(thenBlock->type == AST_BLOCK);
     assert(thenBlock->as.block.count == 1);

     const AstNode *thenReturn = thenBlock->as.block.stmts[0];
     assert(thenReturn != NULL);
     assert(thenReturn->type == AST_RETURN);
     assert(thenReturn->as.ret.expr != NULL);
     assert(thenReturn->as.ret.expr->type == AST_LITERAL_INT);
     assert(thenReturn->as.ret.expr->as.intValue == 1);

     const AstNode *elseBlock = ifNode->as.ifStatement.elseBlock;
     assert(elseBlock != NULL);
     assert(elseBlock->type == AST_BLOCK);
     assert(elseBlock->as.block.count == 1);

     const AstNode *elseReturn = elseBlock->as.block.stmts[0];
     assert(elseReturn != NULL);
     assert(elseReturn->type == AST_RETURN);
     assert(elseReturn->as.ret.expr != NULL);
     assert(elseReturn->as.ret.expr->type == AST_LITERAL_INT);
     assert(elseReturn->as.ret.expr->as.intValue == 0);

     ast_free(program);
   }
/**
 * @brief Runs all parser analysis tests
 */
void test_parser(void) {
    test_prolog_validity();
    test_function_parameters();
    test_main_annotation();
    test_block_statements();
    test_semicolon_statements();
    test_call_statement();
    test_if_else();
}
