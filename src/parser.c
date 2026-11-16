/**
 * @file    parser.c
 * @author  xhorakj01
 * @brief   Implementation of parser
 */

#include "parser.h"
#include "ast.h"
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
  while (pToken.type == TOKEN_NEWLINE) {
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
  advance();
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
  }
  return false;
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
  case KW_BOOLEAN:
    advance();
    return T_BOOLEAN;
  default:
    exit(2);
  }
}
/**
 * @brief Reads an identifier token and returns a heap copy of its name
 *
 * @return strdup'ed identifier name
 */
static char *parse_identifier(void) {
  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }

  char *name = strdup(pToken.value.identifier);
  if (name == NULL) {
    exit(99);
  }

  advance();
  return name;
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

static AstNode *parse_block(void);
static AstNode *parse_statement(void);
static AstNode *parse_body(void);

/**
 * @brief Parses the while loop
 *
 * @details Grammar: while → while expr do NEWLINE INDENT block DEDENT
 * @return AstNode*
 */
static AstNode *parse_while(void) {
  int line = pToken.line;

  // Checking if the keyword is "while"
  expect_keyword(KW_WHILE);

  AstNode *condition = precedence_parse(pInput, &pToken, NULL, NULL);
  if (condition == NULL) {
    exit(2);
  }

  // Now expecting" KW_TO NEWLINE, INDENT
  expect_keyword(KW_DO);
  expect_type(TOKEN_NEWLINE);
  expect_type(TOKEN_INDENT);
  AstNode *body = parse_block();

  expect_type(TOKEN_DEDENT);
  return ast_new_while(condition, body, line);
}

/**
 * @brief Parses the for loop (CONTROL extension)
 *
 * @details Grammar:
 *   for id <- expr (to|until) expr [by expr] [if expr]* do body
 *   or the multi-line header form where the generator and each guard
 *   sit on their own line inside an indented section and 'do' shares
 *   the indentation of 'for'. 'to' is inclusive, 'until' exclusive,
 *   'by' sets the step. to/until/by arrive as plain identifiers.
 *
 * @return AstNode*
 */
static AstNode *parse_for(void) {
  static const char *const stopBounds[] = {"to", "until", NULL};
  static const char *const stopStep[] = {"by", NULL};

  int line = pToken.line;
  expect_keyword(KW_FOR);

  // Multi-line header: generator and guards on their own indented lines
  bool multiline = false;
  if (pToken.type == TOKEN_NEWLINE) {
    multiline = true;
    advance();
    expect_type(TOKEN_INDENT);
  }

  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }
  char *varName = strdup(pToken.value.identifier);
  if (varName == NULL) {
    exit(99);
  }
  advance();

  expect_type(TOKEN_LARROW);

  AstNode *from = precedence_parse(pInput, &pToken, NULL, stopBounds);
  if (from == NULL) {
    exit(2);
  }

  // to (inclusive) or until (exclusive), both plain identifiers
  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }
  bool inclusive;
  if (strcmp(pToken.value.identifier, "to") == 0) {
    inclusive = true;
  } else if (strcmp(pToken.value.identifier, "until") == 0) {
    inclusive = false;
  } else {
    exit(2);
  }
  advance();

  AstNode *toExpr = precedence_parse(pInput, &pToken, NULL, stopStep);
  if (toExpr == NULL) {
    exit(2);
  }

  AstNode *step = NULL;
  if (pToken.type == TOKEN_IDENTIFIER &&
      strcmp(pToken.value.identifier, "by") == 0) {
    advance();
    step = precedence_parse(pInput, &pToken, NULL, NULL);
    if (step == NULL) {
      exit(2);
    }
  }

  if (multiline) {
    expect_type(TOKEN_NEWLINE);
  }

  // Guards: each starts with the 'if' keyword, Boolean expression follows
  size_t guardCount = 0;
  size_t guardCapacity = 0;
  AstNode **guards = NULL;
  while (pToken.type == TOKEN_KEYWORD && pToken.value.keyword == KW_IF) {
    advance();
    AstNode *guard = precedence_parse(pInput, &pToken, NULL, NULL);
    if (guard == NULL) {
      exit(2);
    }

    if (guardCount == guardCapacity) {
      guardCapacity = guardCapacity == 0 ? 4 : guardCapacity * 2;
      AstNode **tmp =
          (AstNode **)realloc((void *)guards, guardCapacity * sizeof(*guards));
      if (tmp == NULL) {
        exit(99);
      }
      guards = tmp;
    }
    guards[guardCount++] = guard;

    if (multiline) {
      expect_type(TOKEN_NEWLINE);
    }
  }

  if (multiline) {
    expect_type(TOKEN_DEDENT);
  }
  expect_keyword(KW_DO);

  AstNode *body = parse_body();

  return ast_new_for(varName, from, toExpr, step, inclusive, guards, guardCount,
                     body, line);
}

/**
 * @brief Parses one body: an indented block
 *        (NEWLINE INDENT block DEDENT) or a single-line statement.
 *        Used for if/else branches and for the do body.
 *
 * @return AstNode* block or single statement node
 */
static AstNode *parse_body(void) {
  if (pToken.type == TOKEN_NEWLINE) {
    advance();
    expect_type(TOKEN_INDENT);
    AstNode *block = parse_block();
    expect_type(TOKEN_DEDENT);
    return block;
  }
  return parse_statement();
}

/**
 * @brief Parser of the if statement / if expression
 *
 * @param elseRequired true when used as an expression (else is mandatory)
 * @return AstNode*
 */
static AstNode *parse_if(bool elseRequired) {
  int line = pToken.line;
  expect_keyword(KW_IF);

  AstNode *condition = precedence_parse(pInput, &pToken, NULL, NULL);
  if (condition == NULL) {
    exit(2);
  }

  expect_keyword(KW_THEN);
  AstNode *thenBlock = parse_body();

  AstNode *elseBlock = NULL;
  if (pToken.type == TOKEN_KEYWORD && pToken.value.keyword == KW_ELSE) {
    advance();
    if (pToken.type == TOKEN_KEYWORD && pToken.value.keyword == KW_IF) {
      // else if -> desugared to a nested if in the else branch
      elseBlock = parse_if(elseRequired);
    } else {
      elseBlock = parse_body();
    }
  } else if (elseRequired) {
    exit(2);
  }

  return ast_new_if(condition, thenBlock, elseBlock, line);
}
static AstNode *parse_expr_or_call(void);
static AstNode **parse_call_arguments(size_t *outCount);

/**
 * @briefParses the id depending on the state. States: assign, statement,
 * function call
 *
 * @return AstNode*
 */
static AstNode *parse_id_stmt(void) {
  /*  3 possible formating states
   *  1) x = 8 assign
   *  2) x - 8 statement
   *  3) x(1-z)fun call
   */
  // Look ahead is necceceary to deside the state
  char *name = strdup(pToken.value.identifier);
  if (name == NULL) {
    exit(99);
  }
  int line = pToken.line;
  advance();

  TokenType tType = pToken.type;
  if (tType == TOKEN_ASSIGN) {
    advance();
    return ast_new_assign(name, parse_expr_or_call(), line);
  }

  if (tType == TOKEN_OPENING_BRACKET) {
    advance();
    size_t argCount = 0;
    AstNode **args = parse_call_arguments(&argCount);
    expect_type(TOKEN_CLOSING_BRACKET);
    return ast_new_expr_statement(ast_new_func_call(name, args, argCount, line),
                                  line);
  }

  // Expression statement starting with identifier: x - 8
  AstNode *first = ast_new_identifier(name, line);
  AstNode *expr = precedence_parse(pInput, &pToken, first, NULL);
  if (expr == NULL) {
    exit(2);
  }
  return ast_new_expr_statement(expr, line);
}

/**
 * @brief Parses the declaraion of variable val(unmodifiable) or var(modifiable)
 *
 * @details varDecl → (val | var) id [ : type ] = exprOrCall
 * @return AstNode*
 */
static AstNode *parse_var_decl(void) {
  int line = pToken.line;

  // Is it var - modifiable?
  bool modifiable = false;
  if (pToken.value.keyword == KW_VAR) {
    modifiable = true;
  }

  advance();
  if (pToken.type != TOKEN_IDENTIFIER) {
    exit(2);
  }

  // saving value identifier - advance would remove
  char *name = strdup(pToken.value.identifier);
  advance();

  DataType declaredType = T_UNIT;
  bool hasDeclaredType = false;

  if (accept_type(TOKEN_COLON)) {
    declaredType = check_type();
    hasDeclaredType = true;
  }

  // Then must come =
  expect_type(TOKEN_ASSIGN);
  AstNode *initAst = parse_expr_or_call();

  return ast_new_var_decl(name, modifiable, declaredType, hasDeclaredType,
                          initAst, line);
}
/**
 * @brief Parses the "return" keyword
 *
 * @return AstNode*
 */
static AstNode *parse_return(void) {
  expect_keyword(KW_RETURN);

  int line = pToken.line;
  if (pToken.type == TOKEN_NEWLINE || pToken.type == TOKEN_DEDENT) {
    return ast_new_return(NULL, line);
  }
  return ast_new_return(parse_expr_or_call(), line);
}

/**
 * @brief
 *
 * @description grammar: varDecl → (val|var) id [: type] = exprOrCall
 * @return AstNode*
 */
static AstNode **parse_call_arguments(size_t *outCount) {
  size_t capacity = 8;
  size_t count = 0;

  AstNode **arguments = (AstNode **)malloc(capacity * sizeof(*arguments));
  if (arguments == 0) {
    exit(99);
  }

  // If the first token is closing braket -> argument count = 0
  if (pToken.type == TOKEN_CLOSING_BRACKET) {
    *outCount = 0;
    free((void *)arguments);
    return NULL;
  }

  while (true) {
    AstNode *argument;
    switch (pToken.type) {
    case TOKEN_IDENTIFIER:
      argument =
          ast_new_identifier(strdup(pToken.value.identifier), pToken.line);
      break;
    case TOKEN_DOUBLE:
      argument = ast_new_double(pToken.value.numberDouble, pToken.line);
      break;
    case TOKEN_INT:
      argument = ast_new_int(pToken.value.numberInt, pToken.line);
      break;
    case TOKEN_STRING:
      argument = ast_new_string(strdup(pToken.value.string), pToken.line);
      break;
    case TOKEN_BOOL:
      argument = ast_new_bool(pToken.value.boolean, pToken.line);
      break;
    default:
      exit(2);
    }

    if (argument == NULL) {
      exit(99);
    }

    // Moving on next token
    advance();

    // VALMEM: member access chain `arg.id` desugars to a builtin call
    while (pToken.type == TOKEN_DOT) {
      advance();
      if (pToken.type != TOKEN_IDENTIFIER) {
        exit(2);
      }
      argument =
          ast_new_member_call(argument, strdup(pToken.value.identifier),
                              pToken.line);
      advance();
    }

    // Assigning the argument to the arguments array
    arguments[count] = argument;
    count++;

    // Reallocating if the arguments array is full
    if (accept_type(TOKEN_COMMA)) {
      if (count == capacity) {
        capacity = capacity * 2;
        AstNode **temp = (AstNode **)realloc((void *)arguments,
                                             capacity * sizeof(*arguments));
        if (temp == 0) {
          exit(99);
        }

        arguments = temp;
      }
      continue;
    }
    break;
  }
  *outCount = count;

  return arguments;
}

/**
 * @brief Helper function, decides whenether the right size of '=' is a
 * expression or call of fun
 *
 * @return AstNode*
 */
static AstNode *parse_expr_or_call(void) {
  // CONTROL: if used as an expression (else branch is mandatory)
  if (pToken.type == TOKEN_KEYWORD && pToken.value.keyword == KW_IF) {
    return parse_if(true);
  }

  if (pToken.type != TOKEN_IDENTIFIER) {
    // If the token is not a identifier, then just call the precedence parser
    AstNode *expression = precedence_parse(pInput, &pToken, NULL, NULL);

    if (expression == NULL) {
      exit(2);
    }
    return expression;
  }

  // Look ahead - expression can be = x-1 or = x(1)
  char *name = strdup(pToken.value.identifier);
  int line = pToken.line;

  if (name == NULL) {
    exit(99);
  }
  advance();

  if (pToken.type != TOKEN_OPENING_BRACKET) {
    // Expression is in format of = x-1...
    AstNode *first = ast_new_identifier(name, line);
    AstNode *expr = precedence_parse(pInput, &pToken, first, NULL);
    if (expr == NULL) {
      exit(2);
    }
    return expr;
  }

  // In the other case, expression is in format of = x(2)
  // call: id ( args )
  advance();

  size_t argumentCount = 0;
  AstNode **arguments = parse_call_arguments(&argumentCount);
  expect_type(TOKEN_CLOSING_BRACKET);
  return ast_new_func_call(name, arguments, argumentCount, line);
}

/**
 * @brief Function looks at pToken and chooses the correct type of command
 *
 * @return AstNode*
 */
static AstNode *parse_statement(void) {
  switch (pToken.type) {
  case TOKEN_KEYWORD: {
    Keyword keyword = pToken.value.keyword;
    switch (keyword) {
    case KW_VAL:
    case KW_VAR:
      return parse_var_decl();
    case KW_IF:
      return parse_if(false);
    case KW_WHILE:
      return parse_while();
    case KW_FOR:
      return parse_for();
    case KW_RETURN:
      return parse_return();
    default:
      exit(2);
    }
  }
  case TOKEN_IDENTIFIER:
    return parse_id_stmt();
  default: {
    int line = pToken.line;
    AstNode *expression = precedence_parse(pInput, &pToken, NULL, NULL);
    if (expression == NULL) {
      exit(2);
    }
    return ast_new_expr_statement(expression, line);
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
  AstNode **stmts = (AstNode **)malloc(capacity * sizeof *stmts);

  if (stmts == NULL) {
    exit(99);
  }

  // Loop of the block + skipping the separators (newlines and semicolons)
  while (true) {
    skip_newlines();
    while (accept_type(TOKEN_SEMICOLON)) {
      skip_newlines();
    }
    // If dedent is detected -> the block ended
    if (pToken.type == TOKEN_DEDENT) {
      break;
    }
    if (count == capacity) {
      capacity *= 2;
      AstNode **tmp =
          (AstNode **)realloc((void *)stmts, capacity * sizeof *stmts);
      if (tmp == NULL) {
        exit(99);
      }
      stmts = tmp;
    }
    AstNode *statement = parse_statement();
    stmts[count++] = statement;
    if (statement->type != AST_IF && statement->type != AST_WHILE &&
        statement->type != AST_FOR && pToken.type != TOKEN_NEWLINE &&
        pToken.type != TOKEN_SEMICOLON && pToken.type != TOKEN_DEDENT) {
      exit(2);
    }
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

  /* Optional @main annotation before def (may sit on its own line) */
  bool isMain = false;
  if (accept_type(TOKEN_AT)) {
    if (pToken.type != TOKEN_IDENTIFIER ||
        strcmp(pToken.value.identifier, "main") != 0) {
      exit(2);
    }
    isMain = true;
    advance();
    skip_newlines();
  }

  expect_keyword(KW_DEF);

  char *name = parse_identifier();

  expect_type(TOKEN_OPENING_BRACKET);
  uint16_t paramCount = 0;
  Parameter *parameters = parse_params(&paramCount);
  expect_type(TOKEN_CLOSING_BRACKET);

  DataType returnType = T_UNIT;
  if (accept_type(TOKEN_COLON)) {
    returnType = check_type();
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
    body = parse_expr_or_call();
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
  expect_type(TOKEN_NEWLINE);

  size_t capacity = 8;
  size_t count = 0;

  AstNode **functions = (AstNode **)malloc(capacity * sizeof *functions);
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
      AstNode **tmp =
          (AstNode **)realloc((void *)functions, capacity * sizeof *functions);
      if (tmp == NULL) {
        exit(99);
      }
      functions = tmp;
    }
    functions[count++] = parse_fun_definition();
  }

  return ast_new_program(functions, count, line);
}
