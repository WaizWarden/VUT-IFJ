/**
 * @file    scanner.h
 * @author  xjuricl01
 * @brief   Scanner header file
 */

typedef enum {
  TOKEN_IDENTIFICATOR,
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

} TokenType;

typedef enum {
  KW_DOUBLE,
  KW_INT,
  KW_STRING,
  KW_VAL,
  KW_VAR,

} Keyword;

typedef union {
  Keyword keyword;
  int numberInt;
  double numberDouble;
  char *string;
  char *identifier;

} TokenValue;

typedef struct {
  TokenType type;
  TokenValue value;

} Token;
