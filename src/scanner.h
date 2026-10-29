/**
 * @file    scanner.h
 * @author  xjuricl01
 * @brief   Scanner header file
 */
#include <stdio.h>

typedef enum {
  TOKEN_IDENTIFIER, /** Uses strdup() - muse use free() ; Stored data in
                       token.value.identifier*/
  TOKEN_KEYWORD,    /** Stored data in token.value.keyword */

  TOKEN_INT,    /** Stored data in token.value.numberInt */
  TOKEN_DOUBLE, /** Stored data in token.value.numberDouble */
  TOKEN_STRING, /** Uses strdup() - muse use free() ; Stored data in
                   token.value.string*/

  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_DIVIDE,
  TOKEN_MULTIPLY,

  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_ASSIGN,
  TOKEN_OPENING_BRACKET,
  TOKEN_CLOSING_BRACKET,
  TOKEN_EQ,
  TOKEN_NOT_EQ,
  TOKEN_NOT,
  TOKEN_GREAER,
  TOKEN_LOWER,
  TOKEN_LOWER_EQ,
  TOKEN_GREATER_EQ,

  TOKEN_EOF,
  TOKEN_NEWLINE,
  TOKEN_ERROR,

  TOKEN_DOT,

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
  KW_IMPORT,
  KW_IF,
  KW_ELSE,
  KW_THEN,
  KW_DO,
  KW_WHILE
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