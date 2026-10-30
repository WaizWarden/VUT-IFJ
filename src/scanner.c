/**
 * @file    scanner.c
 * @author  xjuricl01
 * @brief   Scanner implementation
 */

#include "scanner.h"
#include "scanner_stack.h"
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_STR_LEN 1024
#define REALLOC_CONST 10
#define LEXER_ERROR 1
#define INTERPRET_ERROR 99
#define BASE_TEN 10

int is_keyword(char *str);
bool number_token_end(int currentChar);
int calculateIndent(FILE *input, int *error);

// All possible states in FSM
typedef enum
{
  S_START,
  S_STRING,
  S_ID,
  S_INT,
  S_DOUBLE,
  S_MULTI_LINE_STRING,
} State;

/* AI usage - Gemini
    AI was used here to give me a hint on how to create a struct to convert
   string to keyword
*/
typedef struct
{
  const char *name;
  Keyword keyword;
} KeywordEntry;

KeywordEntry keywordTable[] = {
    {"Double", KW_DOUBLE},
    {"Int", KW_INT},
    {"String", KW_STRING},
    {"val", KW_VAL},
    {"var", KW_VAR},
    {"if", KW_IF},
    {"else", KW_ELSE},
    {"then", KW_THEN},
    {"while", KW_WHILE},
    {"do", KW_DO},
    {"import", KW_IMPORT},
};
//  End of AI usage

// TODO
/*
reformat the comment detection as function - DRY
*/

static int bracketDepth = 0;
static bool stackInitialized = false;
bool lineStart = true;

/**
 * @brief Parses stdin and creates token
 *
 * @param[in,out] input Pointer to stdin
 * @returns Next parsed token
 */
Token get_token(FILE *input)
{
  Token token;
  State state = S_START;

  int currentChar;
  char *str = malloc(INITIAL_STR_LEN);
  size_t capcaity = INITIAL_STR_LEN;
  size_t len = 0;

  if (str == NULL)
  {
    token.type = TOKEN_ERROR;
    fprintf(stderr, "Lexer error: %d, str malloc failed\n", INTERPRET_ERROR);
    goto return_token;
  }

  if (!stackInitialized)
  {
    scanner_stack_init();
    scanner_stack_push(0);
    stackInitialized = true;
  }

  // indent calculation
  int error = 0;
  int spaces = calculateIndent(input, &error);
  if (spaces == -1)
  {
    if (error == -1)
    {

      token.type = TOKEN_ERROR;
      goto return_token;
    }
    else if (error == 2)
    {

      token.type = TOKEN_EOF;
      goto return_token;
    }
    else if (error == 1)
    {

      token.type = TOKEN_NEWLINE;
      goto return_token;
    }
    else
    {
      fprintf(stderr, "Lexer error: %d, Unknown error in indent calculation",
              LEXER_ERROR);
      token.type = TOKEN_ERROR;
      goto return_token;
    }
  }

  while (true)
  {

    // realloc test
    if (len + REALLOC_CONST >= capcaity)
    {
      char *tmp = realloc(str, capcaity * 2);
      if (tmp == NULL)
      {
        fprintf(stderr, "Lexer error: %d, tmp malloc failed\n",
                INTERPRET_ERROR);

        token.type = TOKEN_ERROR;
        goto return_token;
      }
      str = tmp;
      capcaity *= 2;
    }

    currentChar = fgetc(input);

    switch (state)
    {

    case S_START:
      if (currentChar == EOF)
      {
        token.type = TOKEN_EOF;
        goto return_token;
      }

      if (currentChar == '\n')
      {
        if (bracketDepth == 0)
        {
          token.type = TOKEN_NEWLINE;
          goto return_token;
        }

        lineStart = true;
        break;
      }

      if (isspace(currentChar))
      {
        break;
      }

      if (currentChar == '+')
      {
        token.type = TOKEN_PLUS;
        goto return_token;
      }

      if (currentChar == '-')
      {
        token.type = TOKEN_MINUS;
        goto return_token;
      }

      if (currentChar == '*')
      {
        token.type = TOKEN_MULTIPLY;
        goto return_token;
      }

      if (currentChar == '/')
      {
        int next = fgetc(input);

        // One-line comment
        if (next == '/')
        {
          while (next != '\n' && next != EOF)
          {
            next = fgetc(input);
          }
          break;
        }

        // Multi-line comment
        if (next == '*')
        {
          int depth = 1;
          int prev = 0;

          while (depth > 0)
          {
            next = fgetc(input);
            if (next == EOF)
            {
              fprintf(stderr, "Lexer error: %d, Never ending block comment\n",
                      LEXER_ERROR);
              token.type = TOKEN_ERROR;
              goto return_token;
            }

            if (prev == '/' && next == '*')
            {
              depth++;
              prev = 0;
            }

            else if (prev == '*' && next == '/')
            {
              depth--;
              prev = 0;
            }

            else
            {
              prev = next;
            }
          }
          break;
        }

        // not a comment
        ungetc(next, input);
        token.type = TOKEN_DIVIDE;
        goto return_token;
      }

      if (currentChar == '=')
      {
        int next = fgetc(input);

        if (next == '=')
        {
          token.type = TOKEN_EQ;
          goto return_token;
        }
        ungetc(next, input);
        token.type = TOKEN_ASSIGN;
        goto return_token;
      }

      if (currentChar == '>')
      {
        int next = fgetc(input);

        if (next == '=')
        {
          token.type = TOKEN_GREATER_EQ;
          goto return_token;
        }
        ungetc(next, input);
        token.type = TOKEN_GREAER;
        goto return_token;
      }

      if (currentChar == '<')
      {
        int next = fgetc(input);

        if (next == '=')
        {
          token.type = TOKEN_LOWER_EQ;
          goto return_token;
        }
        ungetc(next, input);
        token.type = TOKEN_LOWER;
        goto return_token;
      }

      if (currentChar == '!')
      {
        int next = fgetc(input);

        if (next == '=')
        {
          token.type = TOKEN_NOT_EQ;
          goto return_token;
        }
        ungetc(next, input);
        token.type = TOKEN_NOT;
        goto return_token;
      }

      if (currentChar == ':')
      {
        token.type = TOKEN_COLON;
        goto return_token;
      }

      if (currentChar == ';')
      {
        token.type = TOKEN_SEMICOLON;
        goto return_token;
      }

      if (currentChar == '(')
      {
        bracketDepth++;
        token.type = TOKEN_OPENING_BRACKET;
        goto return_token;
      }

      if (currentChar == ')')
      {
        if (bracketDepth == 0)
        {
          fprintf(stderr, "Error %d, Invalid closing bracket\n", LEXER_ERROR);
          token.type = TOKEN_ERROR;
          goto return_token;
        }
        bracketDepth--;
        token.type = TOKEN_CLOSING_BRACKET;
        goto return_token;
      }

      if (currentChar == '.')
      {
        int next = fgetc(input);

        if (next == '.')
        {
          fprintf(stderr, "Lexer error: %d, invalid double dot\n", LEXER_ERROR);
          token.type = TOKEN_ERROR;
          goto return_token;
        }

        else if (isdigit(next))
        {
          str[len] = '.';
          len++;
          str[len] = (char)next;
          len++;

          state = S_DOUBLE;
        }

        else
        {
          token.type = TOKEN_DOT;
          goto return_token;
        }

        break;
      }

      if (isalpha(currentChar) || currentChar == '_')
      {
        state = S_ID;
        str[len] = (char)currentChar;
        len++;
        break;
      }

      if (isdigit(currentChar))
      {
        state = S_INT;
        str[len] = (char)currentChar;
        len++;

        break;
      }

      if (currentChar == '"')
      {
        state = S_STRING;
        break;
      }

      fprintf(stderr, "Unknown character '%c' \n", currentChar);
      token.type = TOKEN_ERROR;
      goto return_token;

      break;

    case S_STRING:

      if (currentChar == '\n')
      {
        fprintf(stderr, "Lexer error: %d, \\n detected in single line string\n",
                LEXER_ERROR);

        token.type = TOKEN_ERROR;
        goto return_token;
      }

      else if (currentChar == EOF)
      {
        fprintf(stderr, "Lexer error: %d, Invalid EOF single line string\n",
                LEXER_ERROR);
        token.type = TOKEN_ERROR;
        goto return_token;
      }

      // escape chars
      else if (currentChar == '\\')
      {
        // ungetc not needed
        int next = fgetc(input);

        if (next == 'n')
        {
          str[len] = '\n';
        }

        else if (next == '\\')
        {
          str[len] = '\\';
        }

        else if (next == '"')
        {
          str[len] = '\"';
        }

        else if (next == 't')
        {
          str[len] = '\t';
        }

        else if (next == 'r')
        {
          str[len] = '\r';
        }

        else
        {
          fprintf(stderr, "Lexer error: %d, Invalid escape char \n",
                  LEXER_ERROR);
          token.type = TOKEN_ERROR;
          goto return_token;
        }

        len++;
        continue;
      }

      // string end / move to multi-line string
      else if (currentChar == '"')
      {

        // third "
        int next = fgetc(input);
        if (next == '"')
        {
          state = S_MULTI_LINE_STRING;
        }
        else
        {

          ungetc(next, input);
          token.type = TOKEN_STRING;
          str[len] = '\0';
          token.value.string = strdup(str);
          goto return_token;
        }
      }

      // Write string to arr
      else
      {
        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_MULTI_LINE_STRING:
      if (currentChar == EOF)
      {
        fprintf(stderr, "Lexer error: %d, Invalid EOF in multi-line string\n",
                LEXER_ERROR);
        token.type = TOKEN_ERROR;
        goto return_token;
      }

      else if (currentChar == '"')
      {
        int next1 = fgetc(input);
        int next2 = fgetc(input);

        if (next1 == '"' && next2 == '"')
        {
          str[len] = '\0';
          token.type = TOKEN_STRING;
          token.value.string = strdup(str);
          goto return_token;
        }
        ungetc(next2, input);
        ungetc(next1, input);

        str[len] = (char)currentChar;
        len++;
      }
      else
      {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_INT:

      if (!isdigit(currentChar))
      {
        // check for double __
        if (currentChar == '_')
        {
          int next = fgetc(input);

          if (!isdigit(next))
          {
            fprintf(stderr, "Lexer error: %d, double _ in int\n", LEXER_ERROR);

            token.type = TOKEN_ERROR;
            goto return_token;
          }
          ungetc(next, input);
        }

        else if (currentChar == 'e' || currentChar == 'E' ||
                 currentChar == '.')
        {
          state = S_DOUBLE;

          str[len] = (char)currentChar;
          len++;

          break;
        }

        else if (number_token_end(currentChar))
        {

          ungetc(currentChar, input);
          str[len] = '\0';

          // len > 1 because we want to pass "0" not "01"
          if (str[0] == '0' && len > 1)
          {
            fprintf(stderr, "Error: %d, Int cant start with leading zero\n",
                    LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }

          /* AI usage - Gemini
            AI was used here to move from atoi to strtol and explain
            difference Prompt: whats the difference between atoi and strtol,
            convert it to strtol
          */
          char *end;
          int val = (int)strtol(str, &end, BASE_TEN);

          if (end == str || *end != '\0' || errno == ERANGE)
          {
            fprintf(stderr, "Error while converting to integer\n");
            token.type = TOKEN_ERROR;
            goto return_token;
          }
          //  End of AI snippet

          token.type = TOKEN_INT;
          token.value.numberInt = val;
          goto return_token;
        }

        else
        {
          fprintf(stderr, "Lexer error: %d, invalid char detected in integer\n",
                  LEXER_ERROR);
          token.type = TOKEN_ERROR;
          goto return_token;
        }
      }
      else
      {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_DOUBLE:

      // Check for multiple E e . and invalid chars
      if (!isdigit(currentChar))
      {
        if (currentChar == 'e' || currentChar == 'E' || currentChar == '.')
        {
          for (size_t i = 0; i < len; i++)
          {
            if (str[i] == 'e' || str[i] == 'E' ||
                (currentChar == '.' && str[i] == '.'))
            {
              fprintf(stderr,
                      "Error: %d, multiple exponents in double format\n",
                      LEXER_ERROR);
              token.type = TOKEN_ERROR;
              goto return_token;
            }
          }

          // e/E/. cant be next to each other
          if (!isdigit(str[len - 1]))
          {
            fprintf(stderr, "Error: %d, exponents and dot next to each other\n",
                    LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }

          str[len++] = (char)currentChar;
        }

        // +- after e/E
        else if ((currentChar == '+' || currentChar == '-') &&
                 (str[len - 1] == 'e' || str[len - 1] == 'E'))
        {

          str[len] = (char)currentChar;
          len++;
        }

        // Underscore between digits
        else if (currentChar == '_')
        {
          int next = fgetc(input);
          if (!isdigit(str[len - 1]) || !isdigit(next))
          {
            fprintf(stderr, "Error: %d, underscore is not between digits\n",
                    LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }
          ungetc(next, input);
        }

        // d / D suffix
        else if (currentChar == 'd' || currentChar == 'D')
        {
          int next = fgetc(input);

          if (next != EOF && (!isdigit(str[len - 1]) || isdigit(next)))
          {
            fprintf(stderr, "Error: %d, invalid suffix\n", LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }

          ungetc(next, input);

          str[len] = '\0';
          token.type = TOKEN_DOUBLE;
          token.value.numberDouble = strtod(str, NULL);
          goto return_token;
        }

        // new token
        else if (number_token_end(currentChar))
        {

          // literal ends on e/E/.
          if (!isdigit(str[len - 1]))
          {
            fprintf(stderr, "Error: %d, double ends on exponent\n",
                    LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }

          ungetc(currentChar, input);
          str[len] = '\0';

          token.type = TOKEN_DOUBLE;
          token.value.numberDouble = strtod(str, NULL);
          goto return_token;
        }

        else
        {
          fprintf(stderr, "Error: %d, Random suffix/unknown error in double\n",
                  LEXER_ERROR);
          token.type = TOKEN_ERROR;
          goto return_token;
        }
      }

      // Write digit
      else
      {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_ID:
      if (isalnum(currentChar) || currentChar == '_')
      {

        str[len] = (char)currentChar;
        len++;
      }

      else
      {
        ungetc(currentChar, input);
        str[len] = '\0';

        // its keyword
        int key = is_keyword(str);
        if (key != -1)
        {
          token.type = TOKEN_KEYWORD;
          token.value.keyword = keywordTable[key].keyword;
        }

        // its id
        else
        {
          if (strcmp(str, "_") == 0)
          {
            fprintf(stderr, "Error: %d, '_' is not valid identifier\n",
                    LEXER_ERROR);
            token.type = TOKEN_ERROR;
            goto return_token;
          }
          token.type = TOKEN_IDENTIFIER;
          token.value.identifier = strdup(str);
        }

        goto return_token;
      }

      break;

    default:
      fprintf(stderr, "'%c' Default case triggered\n", currentChar);
      token.type = TOKEN_ERROR;
      goto return_token;
    }
  }
return_token:
  free(str);
  return token;
}

/**
 * @brief Decides if string is keyword or identifer
 *
 * @param[in] str pointer to string
 * @returns index to keyword_table array if true, -1 if false
 */
int is_keyword(char *str)
{

  int size = sizeof(keywordTable) / sizeof(keywordTable[0]);

  for (int i = 0; i < size; i++)
  {
    if (strcmp(str, keywordTable[i].name) == 0)
    {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Checks if character ends token (Int/Double)
 *
 * @param[in] character from input
 * @returns true if token ends, else false
 */
bool number_token_end(int currentChar)
{
  int result =
      (currentChar == '+' || currentChar == '-' || currentChar == '*' ||
       currentChar == '/' || currentChar == '=' || currentChar == '<' ||
       currentChar == '>' || currentChar == '!' || currentChar == ')' ||
       currentChar == '(' || currentChar == ':' || currentChar == ';' ||
       isspace(currentChar) || currentChar == EOF);

  return (bool)result;
}

/**
 * @brief Calculates indent for control flow, functions, etc.
 *
 * @param[in, out] input stream
 * @param[out] Possible error states: 0 = No error, -1 = Error, 1 = New line, 2
 * = EOF
 * @returns space count, if value is -1, error happened
 */
int calculateIndent(FILE *input, int *error)
{
  *error = 0;
  if (lineStart && bracketDepth == 0)
  {
    int spaces = 0;
    int indentChar;

    while (true)
    {
      indentChar = fgetc(input);

      if (indentChar == ' ')
      {
        spaces++;
      }
      else if (indentChar == '\t')
      {
        fprintf(stderr, "Lexer error: %d, tab at the start of a line\n",
                LEXER_ERROR);

        *error = -1;
        return -1;
      }
      else if (indentChar == '\n')
      {
        *error = 1;
        return -1;
      }
      else if (indentChar == '/')
      {
        int next = fgetc(input);

        // One-line comment
        if (next == '/')
        {
          while (next != '\n' && next != EOF)
          {
            next = fgetc(input);
          }
          continue;
        }

        // Multi-line comment
        if (next == '*')
        {
          int depth = 1;
          int prev = 0;

          while (depth > 0)
          {
            next = fgetc(input);
            if (next == EOF)
            {
              fprintf(stderr, "Lexer error: %d, Never ending block comment\n",
                      LEXER_ERROR);
              *error = -1;
              return -1;
            }

            if (prev == '/' && next == '*')
            {
              depth++;
              prev = 0;
            }

            else if (prev == '*' && next == '/')
            {
              depth--;
              prev = 0;
            }

            else
            {
              prev = next;
            }
          }
          spaces = 0;
          continue;
        }

        // not a comment
        ungetc(next, input);
        ungetc(indentChar, input);
        break;
      }

      else if (indentChar == EOF)
      {
        *error = 2;
        return -1;
      }
      else
      {
        ungetc(indentChar, input);
        return spaces;
      }
    }
    lineStart = false;
  }
  return 0;
}