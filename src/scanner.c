/**
 * @file    scanner.c
 * @author  xjuricl01
 * @brief   Scanner implementation
 */

#include "scanner.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// TODO: REMOVE AND MAKE IT VIA MALLOC
#define TEMPORARY_STR_LEN 1024

int is_keyword(char *str);

// All possible states in FSM
typedef enum {
  S_START,
  S_STRING,
} State;

typedef struct {
  const char *name;
  Keyword keyword;
} KeywordEntry;

KeywordEntry keyword_table[] = {
    {"Double", KW_DOUBLE}, {"Int", KW_INT}, {"String", KW_STRING},
    {"val", KW_VAL},       {"var", KW_VAR},
};

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

      if (isalpha(current_char) || current_char == '_') {
        state = S_STRING;
        str[len] = (char)current_char;
        len++;
        break;
      }

      break;

    case S_STRING:
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