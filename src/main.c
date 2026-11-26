#include "ast.h"
#include "codegen.h"
#include "parser.h"
#include "semantic.h"
#include "stack.h"
#include "symtable.h"
#include <stdio.h>

int main(void) {
  AstNode *program = parser_parse(stdin);
  scanner_destroy();

  if (program == NULL) {
    return 99;
  }

  Symtable symtable = {0};
  if (!st_init(&symtable)) {
    ast_free(program);
    return 99;
  }

  SemanticError error = analyze_semantics(&symtable, program);
  if (error != ERR_OK) {
    fprintf(stderr, "Semantic error: %d\n", (int)error);
    st_free(&symtable);
    ast_free(program);
    return (int)error;
  }

  cg_gen_program(program);

  st_free(&symtable);
  ast_free(program);
  return 0;
}
