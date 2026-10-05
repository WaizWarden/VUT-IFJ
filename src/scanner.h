/**
 * @file    scanner.h
 * @author  xjuricl01
 * @brief   Scanner header file
 */
#include <stdio.h>

typedef enum {
  TOKEN_IDENTIFIER,
  TOKEN_KEYWORD,

  TOKEN_INT,
  TOKEN_DOUBLE,
  TOKEN_STRING,

  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIVIDE,
  TOKEN_MULTIPLY,

  TOKEN_COLON,
  TOKEN_ASSIGN,
  TOKEN_EOF,
  TOKEN_ERROR,

} TokenType;

/**
 * @brief Reserved language keywords
 */
typedef enum {
  KW_DOUBLE,
  KW_INT,
  KW_STRING,
  KW_VAL,
  KW_VAR,

} Keyword;

/**
 * @brief Union holding value of a token
 */
typedef union {
  Keyword keyword;
  int numberInt;
  double numberDouble;
  char *string;
  char *identifier;

} TokenValue;

/**
 * @brief Structure of a token
 */
typedef struct {
  TokenType type;
  TokenValue value;

} Token;

/**
 * @brief Parses stdin and creates token
 *
 * @param[in,out] input Pointer to stdin
 * @returns Next parsed token
 */
Token get_token(FILE *input);