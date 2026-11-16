/**
 * @file    precedence.c
 * @author  xhorakj01
 * @brief   Implementation of precedence
 */

#include "precedence.h"
#include "ast.h"
#include "hashtable.h"
#include "stack.h"
#include <stdlib.h>
#define PRECEDENCE_TERMINAL_COUNT 12

/*
 * Precedence table from IFJ 8 lecture extended with unary minus and
 * logical operators (! && ||). Relational and equality operators are
 * non-associative and mutually exclusive.
 * Priorities (highest first): u-, ! then *, / then +, - then rel
 * then ==, != then && then ||
 */
/**
 * @brief Precedence table, row = topmost terminal on the stack,
 *        column = input terminal. Order matches PrecSymbol:
 *        +-  * /  u-  !  rel  ==!=  &&  ||  (  )  i  $
 */
static const PrecedenceRelation
    precedenceTable[PRECEDENCE_TERMINAL_COUNT][PRECEDENCE_TERMINAL_COUNT] = {
        // +-
        {PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // * /
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // u-
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // !
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // rel
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_EMPTY, PRECEDENCE_EMPTY,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // ==!=
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_EMPTY, PRECEDENCE_EMPTY,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // &&
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // ||
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_LESS,
         PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        // (
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_EQUAL, PRECEDENCE_LESS, PRECEDENCE_EMPTY},
        // )
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_EMPTY,
         PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER},
        // i
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_GREATER,
         PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_EMPTY,
         PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER},
        // $
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS,
         PRECEDENCE_EMPTY, PRECEDENCE_LESS, PRECEDENCE_EMPTY}};

/**
 * @brief
 * @param stackSymbol
 * @param inputSymbol
 */
PrecedenceRelation precedence_get_relation(PrecSymbol stackSymbol,
                                           PrecSymbol inputSymbol) {
  // Checking the range of stackSymbol
  if (stackSymbol < SYM_PLUS_MINUS ||
      stackSymbol >= PRECEDENCE_TERMINAL_COUNT) {
    exit(99);
  }

  // Checking the range of inputSymbol
  if (inputSymbol < SYM_PLUS_MINUS ||
      inputSymbol >= PRECEDENCE_TERMINAL_COUNT) {
    exit(99);
  }

  return precedenceTable[stackSymbol][inputSymbol];
}

static PrecSymbol convert_token_to_symbol(const Token *token,
                                          PrecSymbol prevSymbol) {
  switch (token->type) {
  case TOKEN_PLUS:
    return SYM_PLUS_MINUS;
  case TOKEN_MINUS:
    return (prevSymbol == SYM_TERM || prevSymbol == SYM_RPAREN) ? SYM_PLUS_MINUS
                                                                : SYM_UMINUS;
  case TOKEN_MULTIPLY:
  case TOKEN_DIVIDE:
    return SYM_MUL_DIV;
  case TOKEN_LOWER:
  case TOKEN_LOWER_EQ:
  case TOKEN_GREAER:
  case TOKEN_GREATER_EQ:
    return SYM_REL;
  case TOKEN_EQ:
  case TOKEN_NOT_EQ:
    return SYM_EQ_NEQ;
  case TOKEN_AND:
    return SYM_AND;
  case TOKEN_OR:
    return SYM_OR;
  case TOKEN_NOT:
    return SYM_NOT;
  case TOKEN_OPENING_BRACKET:
    return SYM_LPAREN;
  case TOKEN_CLOSING_BRACKET:
    return SYM_RPAREN;
  case TOKEN_INT:
  case TOKEN_DOUBLE:
  case TOKEN_STRING:
  case TOKEN_IDENTIFIER:
  case TOKEN_BOOL:
    return SYM_TERM;
  case TOKEN_EOF:
  case TOKEN_NEWLINE:
  case TOKEN_SEMICOLON:
  case TOKEN_DEDENT:
    return SYM_DOLLAR;
  case TOKEN_KEYWORD:
    if (token->value.keyword == KW_THEN || token->value.keyword == KW_DO) {
      return SYM_DOLLAR;
    }
    exit(2);
  case TOKEN_ERROR:
    exit(1);
  default:
    exit(2);
  }
}

static StackItem stack_item_create(const Token *token, PrecSymbol symbol) {
  StackItem item;
  item.symbol = symbol;
  item.token = token->type;
  item.node = NULL;

  switch (token->type) {
  case TOKEN_INT:
    item.node = ast_new_int(token->value.numberInt, 0);
    break;
  case TOKEN_DOUBLE:
    item.node = ast_new_double(token->value.numberDouble, 0);
    break;
  case TOKEN_IDENTIFIER:
    item.node = ast_new_identifier(token->value.identifier, 0);
    break;
  case TOKEN_STRING:
    item.node = ast_new_string(token->value.string, 0);
    break;
  case TOKEN_BOOL:
    item.node = ast_new_bool(token->value.boolean, 0);
    break;
  default:
    break;
  }

  return item;
}

static void push_nonterm(AstNode *node) {
  StackItem result = {SYM_NONTERM, TOKEN_EOF, node};
  stack_push(result);
}

static int precedence_simplify(void) {
  StackItem handle[3];
  size_t cnt = 0;

  while (stack_top()->symbol != SYM_HANDLE_START) {
    if (stack_top()->symbol == SYM_DOLLAR) {
      goto syntax_error;
    }
    if (cnt == 3) {
      goto syntax_error;
    }

    handle[cnt] = stack_pop();
    cnt++;
  }
  stack_pop();

  // Converting i to E
  if (cnt == 1) {
    if (handle[0].symbol == SYM_TERM) {
      StackItem result;
      result.symbol = SYM_NONTERM;
      result.token = TOKEN_EOF;
      result.node = handle[0].node;

      stack_push(result);
      return 0;
    }
  }

  // Converting (E) to E
  if (cnt == 3) {
    if (handle[0].symbol == SYM_RPAREN && handle[1].symbol == SYM_NONTERM &&
        handle[2].symbol == SYM_LPAREN) {
      StackItem result;
      result.symbol = SYM_NONTERM;
      result.token = TOKEN_EOF;
      result.node = handle[1].node;

      stack_push(result);
      return 0;
    }
  }

  if (cnt == 3 && handle[0].symbol == SYM_NONTERM &&
      handle[2].symbol == SYM_NONTERM &&
      (handle[1].symbol == SYM_PLUS_MINUS || handle[1].symbol == SYM_MUL_DIV ||
       handle[1].symbol == SYM_REL || handle[1].symbol == SYM_EQ_NEQ ||
       handle[1].symbol == SYM_AND || handle[1].symbol == SYM_OR)) {
    BinaryOperation operation;

    switch (handle[1].token) {
    case TOKEN_PLUS:
      operation = OP_ADD;
      break;
    case TOKEN_MINUS:
      operation = OP_SUB;
      break;
    case TOKEN_MULTIPLY:
      operation = OP_MUL;
      break;
    case TOKEN_DIVIDE:
      operation = OP_DIV;
      break;
    case TOKEN_LOWER:
      operation = OP_LT;
      break;
    case TOKEN_LOWER_EQ:
      operation = OP_LE;
      break;
    case TOKEN_GREAER:
      operation = OP_GT;
      break;
    case TOKEN_GREATER_EQ:
      operation = OP_GE;
      break;
    case TOKEN_EQ:
      operation = OP_EQ;
      break;
    case TOKEN_NOT_EQ:
      operation = OP_NEQ;
      break;
    case TOKEN_AND:
      operation = OP_AND;
      break;
    case TOKEN_OR:
      operation = OP_OR;
      break;
    default:
      exit(99);
    }

    StackItem result;
    result.symbol = SYM_NONTERM;
    result.token = TOKEN_EOF;
    result.node = ast_new_binary(operation, handle[2].node, handle[0].node, 0);
    stack_push(result);

    return 0;
  }

  // Converting u- E to E
  if (cnt == 2 && handle[0].symbol == SYM_NONTERM &&
      handle[1].symbol == SYM_UMINUS) {
    push_nonterm(ast_new_unary(OP_UNARY_MINUS, handle[0].node, 0));
    return 0;
  }

  // Converting ! E to E
  if (cnt == 2 && handle[0].symbol == SYM_NONTERM &&
      handle[1].symbol == SYM_NOT) {
    push_nonterm(ast_new_unary(OP_UNARY_NOT, handle[0].node, 0));
    return 0;
  }

  // Converting () to Unit literal
  if (cnt == 2 && handle[0].symbol == SYM_RPAREN &&
      handle[1].symbol == SYM_LPAREN) {
    push_nonterm(ast_new_unit(0));
    return 0;
  }

syntax_error:
  for (size_t i = 0; i < cnt; i++) {
    ast_free(handle[i].node);
  }

  return 2;
}

static void precedence_free_stack(void) {
  while (stack_size() > 0) {
    StackItem item = stack_pop();
    ast_free(item.node);
  }

  stack_free();
}

AstNode *precedence_parse(FILE *input, Token *currentToken,
                          AstNode *firstOperand) {
  stack_init();

  StackItem bot;
  bot.symbol = SYM_DOLLAR;
  bot.token = TOKEN_EOF;
  bot.node = NULL;
  stack_push(bot);

  PrecSymbol prevSymbol = SYM_DOLLAR;

  if (firstOperand != NULL) {
    push_nonterm(firstOperand);
    prevSymbol = SYM_TERM;
  }

  PrecSymbol inputSymbol = convert_token_to_symbol(currentToken, prevSymbol);

  while (1) {
    if (inputSymbol == SYM_DOLLAR && stack_size() == 2 &&
        stack_top()->symbol == SYM_NONTERM) {
      AstNode *result = stack_pop().node;
      stack_free();
      return result;
    }

    PrecSymbol stackSymbol = stack_top_terminal()->symbol;
    PrecedenceRelation relation =
        precedence_get_relation(stackSymbol, inputSymbol);

    switch (relation) {
    case PRECEDENCE_LESS:
      stack_insert_handle_start();
      /* fall through */
    case PRECEDENCE_EQUAL:
      stack_push(stack_item_create(currentToken, inputSymbol));
      prevSymbol = inputSymbol;
      *currentToken = get_token(input);
      inputSymbol = convert_token_to_symbol(currentToken, prevSymbol);
      break;
    case PRECEDENCE_GREATER:
      if (precedence_simplify() != 0) {
        precedence_free_stack();
        return NULL;
      }
      break;
    case PRECEDENCE_EMPTY:
      precedence_free_stack();
      return NULL;
    }
  }
}
