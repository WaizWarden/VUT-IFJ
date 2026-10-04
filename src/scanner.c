/**
 * @file    scanner.c
 * @author  xjuricl01
 * @brief   Scanner implementation
 */

#include "scanner.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// NOTES:
// Tell parser that it has to manage negative numbers on its own

//  REMOVE AND MAKE IT VIA MALLOC
#define TEMPORARY_STR_LEN 1024
#define LEXER_ERROR 50
#define BASE_TEN 10

int is_keyword(char *str);

// All possible states in FSM
typedef enum {
  S_START,
  S_STRING,
  S_ID,
  S_INT,
} State;

/* AI usage - Gemini
    AI was used here to give me a hint on how to create a struct to convert
   string to keyword
*/
typedef struct {
  const char *name;
  Keyword keyword;
} KeywordEntry;

KeywordEntry keyword_table[] = {
    {"Double", KW_DOUBLE}, {"Int", KW_INT}, {"String", KW_STRING},
    {"val", KW_VAL},       {"var", KW_VAR},
};
//  End of AI usage

/**
 * @brief Parses stdin and creates token
 *
 * @param[in,out] input Pointer to stdin
 * @returns Next parsed token
 */
Token get_token(FILE *input) {
  Token token;
  State state = S_START;

  char str[TEMPORARY_STR_LEN] = "";
  size_t len = 0;

  int current_char;

  while (true) {
    current_char = fgetc(input);

    switch (state) {

    case S_START:
      if (current_char == EOF) {
        token.type = TOKEN_EOF;
        return token;
      }

      if (isspace(current_char)) {
        break;
      }

      if (current_char == '+') {
        token.type = TOKEN_PLUS;
        return token;
      }

      if (current_char == '-') {
        token.type = TOKEN_MINUS;
        return token;
      }

      if (current_char == '*') {
        token.type = TOKEN_MULTIPLY;
        return token;
      }

      if (current_char == '/') {
        token.type = TOKEN_DIVIDE;
        return token;
      }

      if (current_char == '=') {
        token.type = TOKEN_ASSIGN;
        return token;
      }

      if (current_char == ':') {
        token.type = TOKEN_COLON;
        return token;
      }

      if (isalpha(current_char) || current_char == '_') {
        state = S_ID;
        str[len] = (char)current_char;
        len++;
        break;
      }

      if (isdigit(current_char)) {
        state = S_INT;
        str[len] = (char)current_char;
        len++;
        break;
      }

      break;

    case S_INT:

      if (isdigit(current_char)) {
        if (len < sizeof(str) - 1) {
          str[len] = (char)current_char;
          len++;
        }
      }

      // check for double __
      else if ((char)current_char == '_') {
        int next = fgetc(input);

        if (!isdigit(next)) {
          fprintf(stderr, "Lexer error: %d\n", LEXER_ERROR);
        }
        ungetc(next, input);
      }

      else {
        ungetc(current_char, input);
        str[len] = '\0';

        /* AI usage - Gemini
          AI was used here to move from atoi to strtol and explain difference
          Prompt: whats the difference between atoi and strtol, convert it to
          strtol
        */
        char *end;
        int val = (int)strtol(str, &end, BASE_TEN);

        if (end == str || *end != '\0' || errno == ERANGE) {
          fprintf(stderr, "Error while converting to integer");
        }
        //  End of AI snippet

        token.type = TOKEN_INT;
        token.value.numberInt = val;
        return token;
      }

      break;

    case S_ID:
      if (isalnum(current_char) || current_char == '_') {
        if (len < sizeof(str) - 1) {
          str[len] = (char)current_char;
          len++;
        }
      }

      else {
        ungetc(current_char, input);
        str[len] = '\0';

        int key = is_keyword(str);
        if (key != -1) {
          token.type = TOKEN_KEYWORD;
          token.value.keyword = keyword_table[key].keyword;
        }

        else {

          token.type = TOKEN_IDENTIFIER;
          token.value.identifier = strdup(str);
        }

        return token;
      }

      break;

    default:
      printf("'%c' Fell through\n", current_char);
    }
  }
}

/**
 * @brief Decides if string is keyword or identifer
 *
 * @param str pointer to string
 * @returns index to keyword_table array if true, -1 if false
 */
int is_keyword(char *str) {

  int size = sizeof(keyword_table) / sizeof(keyword_table[0]);

  for (int i = 0; i < size; i++) {
    if (strcmp(str, keyword_table[i].name) == 0) {
      return i;
    }
  }

  return -1;
}