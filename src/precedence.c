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
#include <string.h>
#define PRECEDENCE_TERMINAL_COUNT 13

/*
 * Precedence table from IFJ 8 lecture extended with unary minus,
 * logical operators (! && ||) and member access (.). Relational and
 * equality operators are non-associative and mutually exclusive.
 * Priorities (highest first): . then u-, ! then *, / then +, - then
 * rel then ==, != then && then ||
 */
/**
 * @brief Precedence table, row = topmost terminal on the stack,
 *        column = input terminal. Order matches PrecSymbol:
 *        +-  * /  u-  !  rel  ==!=  &&  ||  .  (  )  i  $
 */
static const PrecedenceRelation
    precedenceTable[PRECEDENCE_TERMINAL_COUNT][PRECEDENCE_TERMINAL_COUNT] = {
        // +-
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // * /
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // u-
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // !
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // rel
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_EMPTY,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // ==!=
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_EMPTY,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // &&
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // ||
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // .   (member access: '<' only for the identifier operand,
        //      '>' reduces E.i once the right operand is formed)
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
            PRECEDENCE_LESS,
            PRECEDENCE_GREATER,
        },
        // (
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_EQUAL,
            PRECEDENCE_LESS,
            PRECEDENCE_EMPTY,
        },
        // )
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
        },
        // i
        {
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_GREATER,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
            PRECEDENCE_EMPTY,
            PRECEDENCE_GREATER,
        },
        // $
        {
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_LESS,
            PRECEDENCE_EMPTY,
            PRECEDENCE_LESS,
            PRECEDENCE_EMPTY,
        },
};

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
                                          PrecSymbol prevSymbol,
                                          const char *const *stopIdents) {
  if (token->type == TOKEN_IDENTIFIER && stopIdents != NULL) {
    for (size_t i = 0; stopIdents[i] != NULL; i++) {
      if (strcmp(token->value.identifier, stopIdents[i]) == 0) {
        return SYM_DOLLAR;
      }
    }
  }

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
  case TOKEN_DOT:
    return SYM_DOT;
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
  case TOKEN_INTERP_END:
    return SYM_DOLLAR;
  case TOKEN_KEYWORD:
    if (token->value.keyword == KW_THEN || token->value.keyword == KW_DO ||
        token->value.keyword == KW_ELSE || token->value.keyword == KW_IF ||
        token->value.keyword == KW_FOR) {
      return SYM_DOLLAR;
    }
    exit(2);
  case TOKEN_INTERP_START:
    exit(2);
  case TOKEN_ERROR:
    exit(1);
  default:
    exit(2);
  }
}

AstNode *precedence_read_string(FILE *input, Token *currentToken) {
  int line = currentToken->line;
  AstNode *node = ast_new_string(currentToken->value.string, line);

  *currentToken = get_token(input);

  // When interpolateds started
  while (currentToken->type == TOKEN_INTERP_START) {
    // Innter token getter
    *currentToken = get_token(input);

    // Inner node
    AstNode *innerNode = precedence_parse(input, currentToken, NULL, NULL);
    if (innerNode == NULL) {
      ast_free(node);
      return NULL;
    }
    // End of the current token (TOKEN_INTERP_END)

    AstNode **arguments = (AstNode **)malloc(sizeof(*arguments));
    if (arguments == NULL) {
      ast_free(node);
      exit(99);
    }
    arguments[0] = innerNode;
    AstNode *toStr = ast_new_func_call(strdup("toStr"), arguments, 1, line);
    node = ast_new_binary(OP_ADD, node, toStr, line);

    *currentToken = get_token(input);

    if (currentToken->type != TOKEN_STRING) {
      ast_free(node);
      return NULL;
    }

    AstNode *seg = ast_new_string(currentToken->value.string, line);
    node = ast_new_binary(OP_ADD, node, seg, line);
    *currentToken = get_token(input);
  }

  return node;
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

static void push_nonterm(Stack *stack, AstNode *node) {
  StackItem result = {SYM_NONTERM, TOKEN_EOF, node};
  stack_push(stack, result);
}

static int precedence_simplify(Stack *stack) {
  StackItem handle[3];
  size_t cnt = 0;

  while (stack_top(stack)->symbol != SYM_HANDLE_START) {
    if (stack_top(stack)->symbol == SYM_DOLLAR) {
      goto syntax_error;
    }
    if (cnt == 3) {
      goto syntax_error;
    }

    handle[cnt] = stack_pop(stack);
    cnt++;
  }
  stack_pop(stack);

  // Converting i to E
  if (cnt == 1 && handle[0].symbol == SYM_TERM) {
    StackItem result;
    result.symbol = SYM_NONTERM;
    result.token = TOKEN_EOF;
    result.node = handle[0].node;

    stack_push(stack, result);
    return 0;
  }

  // Converting (E) to E
  if (cnt == 3 && handle[0].symbol == SYM_RPAREN &&
      handle[1].symbol == SYM_NONTERM && handle[2].symbol == SYM_LPAREN) {
    StackItem result;
    result.symbol = SYM_NONTERM;
    result.token = TOKEN_EOF;
    result.node = handle[1].node;

    stack_push(stack, result);
    return 0;
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
    stack_push(stack, result);

    return 0;
  }

  // Converting E . i to E (member access, VALMEM desugar)
  if (cnt == 3 && handle[0].symbol == SYM_NONTERM &&
      handle[1].symbol == SYM_DOT && handle[2].symbol == SYM_NONTERM) {
    if (handle[0].node == NULL || handle[0].node->type != AST_IDENTIFIER) {
      goto syntax_error;
    }

    // steal the member name, release the identifier node shell
    char *memberName = handle[0].node->as.identif;
    free(handle[0].node);

    push_nonterm(stack, ast_new_member_call(handle[2].node, memberName, 0));
    return 0;
  }

  // Converting u- E to E
  if (cnt == 2 && handle[0].symbol == SYM_NONTERM &&
      handle[1].symbol == SYM_UMINUS) {
    push_nonterm(stack, ast_new_unary(OP_UNARY_MINUS, handle[0].node, 0));
    return 0;
  }

  // Converting ! E to E
  if (cnt == 2 && handle[0].symbol == SYM_NONTERM &&
      handle[1].symbol == SYM_NOT) {
    push_nonterm(stack, ast_new_unary(OP_UNARY_NOT, handle[0].node, 0));
    return 0;
  }

  // Converting () to Unit literal
  if (cnt == 2 && handle[0].symbol == SYM_RPAREN &&
      handle[1].symbol == SYM_LPAREN) {
    push_nonterm(stack, ast_new_unit(0));
    return 0;
  }

syntax_error:
  for (size_t i = 0; i < cnt; i++) {
    ast_free(handle[i].node);
  }

  return 2;
}

static void precedence_free_stack(Stack *stack) {
  while (stack_size(stack) > 0) {
    StackItem item = stack_pop(stack);
    ast_free(item.node);
  }

  stack_free(stack);
}

AstNode *precedence_parse(FILE *input, Token *currentToken,
                          AstNode *firstOperand,
                          const char *const stopIdents[]) {
  Stack stack;
  stack_init(&stack);

  StackItem bot;
  bot.symbol = SYM_DOLLAR;
  bot.token = TOKEN_EOF;
  bot.node = NULL;
  stack_push(&stack, bot);

  PrecSymbol prevSymbol = SYM_DOLLAR;

  if (firstOperand != NULL) {
    push_nonterm(&stack, firstOperand);
    prevSymbol = SYM_TERM;
  }

  PrecSymbol inputSymbol =
      convert_token_to_symbol(currentToken, prevSymbol, stopIdents);

  while (1) {
    if (inputSymbol == SYM_DOLLAR && stack_size(&stack) == 2 &&
        stack_top(&stack)->symbol == SYM_NONTERM) {
      AstNode *result = stack_pop(&stack).node;
      stack_free(&stack);
      return result;
    }

    PrecSymbol stackSymbol = stack_top_terminal(&stack)->symbol;
    PrecedenceRelation relation =
        precedence_get_relation(stackSymbol, inputSymbol);

    switch (relation) {
    case PRECEDENCE_LESS:
      stack_insert_handle_start(&stack);
      /* fall through */
    case PRECEDENCE_EQUAL:
      if (inputSymbol == SYM_TERM && currentToken->type == TOKEN_STRING) {
        StackItem item = {SYM_TERM, TOKEN_STRING, NULL};
        item.node = precedence_read_string(input, currentToken);
        if (item.node == NULL) {
          precedence_free_stack(&stack);
          return NULL;
        }
        stack_push(&stack, item);
        // helper už posunul *currentToken za string
      } else {
        stack_push(&stack, stack_item_create(currentToken, inputSymbol));
        *currentToken = get_token(input);
      }
      prevSymbol = inputSymbol;
      inputSymbol =
          convert_token_to_symbol(currentToken, prevSymbol, stopIdents);
      break;
    case PRECEDENCE_GREATER:
      if (precedence_simplify(&stack) != 0) {
        precedence_free_stack(&stack);
        return NULL;
      }
      break;
    case PRECEDENCE_EMPTY:
      precedence_free_stack(&stack);
      return NULL;
    }
  }
}
