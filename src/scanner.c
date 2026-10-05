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

KeywordEntry keywordTable[] = {
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

  int currentChar;

  while (true) {
    currentChar = fgetc(input);

    switch (state) {

    case S_START:
      if (currentChar == EOF) {
        token.type = TOKEN_EOF;
        return token;
      }

      if (isspace(currentChar)) {
        break;
      }

      if (currentChar == '+') {
        token.type = TOKEN_PLUS;
        return token;
      }

      if (currentChar == '-') {
        token.type = TOKEN_MINUS;
        return token;
      }

      if (currentChar == '*') {
        token.type = TOKEN_MULTIPLY;
        return token;
      }

      if (currentChar == '/') {
        token.type = TOKEN_DIVIDE;
        return token;
      }

      if (currentChar == '=') {
        token.type = TOKEN_ASSIGN;
        return token;
      }

      if (currentChar == ':') {
        token.type = TOKEN_COLON;
        return token;
      }

      if (isalpha(currentChar) || currentChar == '_') {
        state = S_ID;
        str[len] = (char)currentChar;
        len++;
        break;
      }

      if (isdigit(currentChar)) {
        if (currentChar == '0') {
          token.type = TOKEN_ERROR;
          return token;
        }
        state = S_INT;
        str[len] = (char)currentChar;
        len++;

        break;
      }

      if (currentChar == '"') {
        state = S_STRING;
        break;
      }

      break;

    case S_STRING:

      if (currentChar == '\n') {
        fprintf(stderr, "Lexer error: %d, \\n detected in single line string\n",
                LEXER_ERROR);

        token.type = TOKEN_ERROR;
        return token;
      }

      if (currentChar == -1) {
        fprintf(stderr, "Lexer error: %d, Invalid EOF detected\n", LEXER_ERROR);

        token.type = TOKEN_ERROR;
        return token;
      }

      if (currentChar == '\\') {
        char next = (char)fgetc(input);

        if (next == 'n')
          str[len] = '\n';

        else if (next == '\\')
          str[len] = '\\';

        else if (next == '"')
          str[len] = '\"';

        else if (next == 't')
          str[len] = '\t';

        else if (next == 'r')
          str[len] = '\r';

        else {
          fprintf(stderr, "Lexer error: %d, Invalid escape char \n",
                  LEXER_ERROR);
          token.type = TOKEN_ERROR;
          return token;
        }

        len++;
        continue;
      }

      if (currentChar == '"') {
        token.type = TOKEN_STRING;
        str[len] = '\0';
        token.value.string = strdup(str);
        return token;
      }

      str[len] = currentChar;
      len++;
      break;

    case S_INT:

      if (isdigit(currentChar)) {
        if (len < sizeof(str) - 1) {
          str[len] = (char)currentChar;
          len++;
        }
      }

      // check for double __
      else if ((char)currentChar == '_') {
        int next = fgetc(input);

        if (!isdigit(next)) {
          fprintf(stderr, "Lexer error: %d, double _ in int\n", LEXER_ERROR);

          token.type = TOKEN_ERROR;
          return token;
        }
        ungetc(next, input);
      }

      else {
        ungetc(currentChar, input);
        str[len] = '\0';

        /* AI usage - Gemini
          AI was used here to move from atoi to strtol and explain difference
          Prompt: whats the difference between atoi and strtol, convert it to
          strtol
        */
        char *end;
        int val = (int)strtol(str, &end, BASE_TEN);

        if (end == str || *end != '\0' || errno == ERANGE) {
          fprintf(stderr, "Error while converting to integer\n");
          token.type = TOKEN_ERROR;
          return token;
        }
        //  End of AI snippet

        token.type = TOKEN_INT;
        token.value.numberInt = val;
        return token;
      }

      break;

    case S_ID:
      if (isalnum(currentChar) || currentChar == '_') {
        if (len < sizeof(str) - 1) {
          str[len] = (char)currentChar;
          len++;
        }
      }

      else {
        ungetc(currentChar, input);
        str[len] = '\0';

        int key = is_keyword(str);
        if (key != -1) {
          token.type = TOKEN_KEYWORD;
          token.value.keyword = keywordTable[key].keyword;
        }

        else {

          token.type = TOKEN_IDENTIFIER;
          token.value.identifier = strdup(str);
        }

        return token;
      }

      break;

    default:
      printf("'%c' Fell through\n", currentChar);
    }
  }
}

/**
 * @brief Decides if string is keyword or identifer
 *
 * @param[in] str pointer to string
 * @returns index to keyword_table array if true, -1 if false
 */
int is_keyword(char *str) {

  int size = sizeof(keywordTable) / sizeof(keywordTable[0]);

  for (int i = 0; i < size; i++) {
    if (strcmp(str, keywordTable[i].name) == 0) {
      return i;
    }
  }

  return -1;
}