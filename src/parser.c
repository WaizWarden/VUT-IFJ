/**
 * @file    parser.c
 * @author  xhorakj01
 * @brief   Implementation of parser
 */

#include "parser.h"
#include "precedence.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *pInput;
static Token pToken;

/**
 * @brief Frees the dynamically allocated memory associated with the current
 * token's value if it is an identifier or a string.
 *
 */
static void token_drop_value(void) {
  if (pToken.type == TOKEN_IDENTIFIER) {
    free(pToken.value.identifier);
  }
  if (pToken.type == TOKEN_STRING) {
    free(pToken.value.string);
  }
}

/**
 * @brief Helper function for advancing the ptoken, checkijgng if the
 * pToken.type is not TOKEN_ERROR
 *
 */
static void advance(void) {
  token_drop_value();

  pToken = get_token(pInput);
  if (pToken.type == TOKEN_ERROR) {
    exit(1);
  }
}
/**
 * @brief Helper function expect_keyword. Used to check for keywords.
 *
 * @param keyword
 */
static void expect_keyword(Keyword keyword) {
  if (pToken.type == TOKEN_KEYWORD) {
    if (pToken.value.keyword == keyword) {
      advance();
    } else {
      exit(2);
    }
  } else {
    exit(2);
  }
}
/**
 * @brief Helper function expect_type. Used to check for types.
 *
 * @param tkType
 */
static void expect_type(TokenType tkType) {
  if (pToken.type == tkType) {
    advance();
  } else {
    exit(2);
  }
}

/**
 * @brief Skips the token of new lines if present
 *
 */
static void skip_newlines(void) {
  if (pToken.type == TOKEN_NEWLINE) {
    advance();
  }
}
/**
 * @brief Checking if the first prolog in the file is present "import ifj26.*".
 * If not, exit(2)
 *
 */
static void check_prolog(void) {
  // every file needs to have "import ifj26.*"
  expect_keyword(KW_IMPORT);
  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }
  int compare = strcmp(pToken.value.identifier, "ifj26");
  if (compare != 0) {
    // the part after "import" is not "ifj26"
    exit(2);
  }
  expect_type(TOKEN_DOT);
  expect_type(TOKEN_MULTIPLY);
}
/**
 * @brief Helper function for telling if some token type is or is not present.
 *
 * @param tkType
 * @return true
 * @return false
 */
static bool accept_type(TokenType tkType) {
  if (pToken.type == tkType) {
    advance();
    return true;
  } else {
    return false;
  }
}
/**
 * @brief Assigning the correct datatype (string, double...)
 *
 * @return DataType
 */
static DataType check_type(void) {
  // Check if the type is not keyword -> error
  if (pToken.type != TOKEN_KEYWORD) {
    exit(2);
  }

  switch (pToken.value.keyword) {
  case KW_INT:
    advance();
    return T_INT;
  case KW_DOUBLE:
    advance();
    return T_DOUBLE;
  case KW_STRING:
    advance();
    return T_STRING;
  case KW_UNIT:
    advance();
    return T_UNIT;
  default:
    exit(2);
  }
}
/**
 * @brief Gets the name of a parse function
 *
 * @param funName
 * @return bool returns if the function is or is not main function
 */
static bool parse_get_fun_name(char **funName) {
  bool isMain = false;

  if (accept_type(TOKEN_AT)) {
    isMain = true;
  }

  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }

  // checking if the function is main and the identifier is "main"
  if (isMain) {
    // copying the function name into the funName variable
    int cmpValue = strcmp(pToken.value.identifier, "main");

    if (cmpValue != 0) {
      // identifier is not equal to "main"
      exit(2);
    }
  }

  // Copying the function name before advance (would delete it)
  *funName = strdup(pToken.value.identifier);
  if (funName == NULL) {
    exit(99);
  }

  advance();
  return isMain;
}
/**
 * @brief Parses every parameter after '(' of the function
 *
 * @param outCount
 * @return Parameter*
 */
static Parameter *parse_params(uint16_t *outCount) {
  size_t paramCount = 0;
  size_t capacity = 8;

  Parameter *params = malloc(capacity * sizeof(Parameter));
  if (params == NULL) {
    exit(99);
  }

  // If there is no parameter. Behind '(' is ')'
  if (pToken.type == TOKEN_CLOSING_BRACKET) {
    *outCount = 0;
    free(params);
    return NULL;
  }

  // Parses param, then continues while ',' follows: param ( , param )*
  do {
    if (paramCount == capacity) {
      capacity *= 2;
      Parameter *tmp = realloc(params, capacity * sizeof *params);
      if (tmp == NULL) {
        exit(99);
      }
      params = tmp;
    }

    if (pToken.type != TOKEN_IDENTIFIER) {
      exit(2);
    }

    params[paramCount].name = strdup(pToken.value.identifier);
    if (params[paramCount].name == NULL) {
      exit(99);
    }

    advance();
    // Semantic analysis will fill this value, not parser
    params[paramCount].uid = 0;

    expect_type(TOKEN_COLON);

    params[paramCount].type = check_type();
    paramCount++;
  } while (accept_type(TOKEN_COMMA));

  *outCount = (uint16_t)paramCount;
  return params;
}

static AstNode *parse_var_decl(void);
static AstNode *parse_if(void);
static AstNode *parse_while(void);
static AstNode *parse_return(void);
static AstNode *parse_id_stmt(void);
static AstNode *parse_expr_or_call(void);

/**
 * @brief Function looks at pToken and chooses the correct type of command
 *
 * @return AstNode*
 */
static AstNode *parse_statement(void) {
  switch (pToken.type) {
  case TOKEN_KEYWORD:
    Keyword kw = pToken.value.keyword;
    switch (kw) {
    case KW_VAL:
    case KW_VAR:
      return parse_var_decl();
    case KW_IF:
      return parse_if();
    case KW_WHILE:
      return parse_while();
    case KW_RETURN:
      return parse_return();
    default:
      exit(2);
    }
  case TOKEN_IDENTIFIER:
    return parse_id_stmt();
  default: {
    AstNode *e = precedence_parse(pInput, &pToken, NULL);
    if (e == NULL)
      exit(2);
    return ast_new_expr_statement(e, pToken.line);
  }
  }
}

/**
 * @brief Returns the AstNode of block
 *
 * @return AstNode*
 */
static AstNode *parse_block(void) {
  int line = pToken.line;
  size_t capacity = 8;
  size_t count = 0;
  AstNode **stmts = malloc(capacity * sizeof *stmts);

  if (stmts == NULL) {
    exit(99);
  }

  // Loop of the block + skipping the new lines
  while (true) {
    skip_newlines();
    // If dedent is detected -> the block ended
    if (pToken.type == TOKEN_DEDENT) {
      break;
    }
    if (count == capacity) {
      /* realloc ×2, exit(99) */
    }
    stmts[count++] = parse_statement();
  }
  if (count == 0) {
    exit(2);
  }
  return ast_new_block(stmts, count, line);
}
/**
 * @brief Checking if the function definition contains all the needed parameters
 * from ast.h
 *
 */
static AstNode *parse_fun_definition(void) {
  int line = pToken.line;
  expect_keyword(KW_DEF);

  char *name = NULL;
  bool isMain = false;
  isMain = parse_get_fun_name(&name);

  expect_type(TOKEN_OPENING_BRACKET);
  uint16_t paramCount = 0;
  Parameter *parameters = parse_params(&paramCount);
  expect_type(TOKEN_CLOSING_BRACKET);

  DataType returnType = T_UNIT;
  if (accept_type(TOKEN_COLON)) {
    returnType = parse_type();
  }

  expect_type(TOKEN_ASSIGN);
  AstNode *body = NULL;
  // the function can be either assigned on the same line like: "def f(x: Int):
  // Int = x * 2" or it can have '=' on one line and rest of the fun bellow with
  // indent (or the fun is invalid)
  if (accept_type(TOKEN_NEWLINE)) {
    // If there is new line, next token must be indent ()
    expect_type(TOKEN_INDENT);
    body = parse_block();
    expect_type(TOKEN_DEDENT);
  } else {
    body = precedence_parse(pInput, &pToken, NULL);
    if (body == NULL) {
      exit(2);
    }
  }

  return ast_new_func_def(name, isMain, returnType, parameters, paramCount,
                          body, line);
}

AstNode *parser_parse(FILE *fileInput) {
  pInput = fileInput;
  advance();
  int line = pToken.line;

  // Skiping the new lines in the beggining of file
  skip_newlines();

  // Checcking the presence of "import ifj26.*
  check_prolog();

  size_t capacity = 8;
  size_t count = 0;

  AstNode **functions = malloc(capacity * sizeof *functions);
  if (functions == 0) {
    exit(99);
  }

  while (true) {
    skip_newlines();
    if (pToken.type == TOKEN_EOF) {
      break;
    }
    if (count == capacity) {
      capacity *= 2;
      AstNode **tmp = realloc(functions, capacity * sizeof *functions);
      if (tmp == NULL) {
        exit(99);
      }
      functions = tmp;
    }
    functions[count++] = parse_fun_definition();
  }

  return ast_new_program(functions, count, line);
}

/**
 * @brief advance parser fun
 */
static void parser_advance(FILE *input, Token *currentToken) {
  *currentToken = get_token(input);

  if (currentToken->type == TOKEN_ERROR) {
    exit(1);
  }
}

AstNode *parser_parse_assignment(FILE *input, Token *currentToken) {
  if (currentToken->type == TOKEN_ERROR) {
    exit(1);
  }

  if (currentToken->type != TOKEN_IDENTIFIER) {
    exit(2);
  }

  char *character = currentToken->value.identifier;
  parser_advance(input, currentToken);

  if (currentToken->type != TOKEN_ASSIGN) {
    free(character);
    exit(2);
  }

  parser_advance(input, currentToken);
  AstNode *value = precedence_parse(input, currentToken, NULL);

  if (value == NULL) {
    free(character);
    exit(2);
  }

  return ast_new_assign(character, value, 0);
}