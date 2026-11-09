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
    assert(source != NULL);

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
 * @brief Runs all parser analysis tests
 */
void test_parser(void) {
    test_prolog_validity();
    test_function_parameters();
}
