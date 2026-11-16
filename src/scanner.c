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
#define SCANNER_ERROR 1
#define INTERPRET_ERROR 99

static int is_keyword(char *str);
static bool number_token_end(int currentChar);
static int calculateIndent(FILE *input, int *error);
static int next_char(FILE *input);
static void put_back(int currentChar, FILE *input);
static void handle_eof(Token *token);

// States in scanner
typedef enum {
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
typedef struct {
  const char *name;
  Keyword keyword;
} KeywordEntry;

KeywordEntry keywordTable[] = {
    {"Double", KW_DOUBLE}, {"Int", KW_INT},       {"String", KW_STRING},
    {"val", KW_VAL},       {"var", KW_VAR},       {"if", KW_IF},
    {"else", KW_ELSE},     {"then", KW_THEN},     {"while", KW_WHILE},
    {"do", KW_DO},         {"import", KW_IMPORT}, {"Unit", KW_UNIT},
    {"def", KW_DEF},       {"return", KW_RETURN}, {"for", KW_FOR},
    {"Boolean", KW_BOOLEAN},
};
//  End of AI usage

static int bracketDepth = 0;
static bool stackInitialized = false;
static int pendingDedents = 0;
static int currentLine = 1;
static bool lineStart = true;
static bool EndOfFileDetected = false;

/**
 * @brief Parses stdin and creates token
 *
 * @param[in,out] input Pointer to stdin
 * @returns Next parsed token
 */
Token get_token(FILE *input) {
  Token token;
  State state = S_START;

  int tokenLine = currentLine;
  int currentChar;

  char *str = malloc(INITIAL_STR_LEN);
  size_t capcaity = INITIAL_STR_LEN;
  size_t len = 0;

  if (str == NULL) {
    token.type = TOKEN_ERROR;
    fprintf(stderr, "Lexer error: %d, str malloc failed\n", INTERPRET_ERROR);
    goto return_token;
  }

  if (!stackInitialized) {
    scanner_stack_init();
    scanner_stack_push(0);
    stackInitialized = true;
  }

  if (pendingDedents > 0) {
    pendingDedents--;
    token.type = TOKEN_DEDENT;
    goto return_token;
  }

  if (EndOfFileDetected) {
    handle_eof(&token);
    goto return_token;
  }

  // indent calculation
  int error = 0;
  bool atLineStart = lineStart && bracketDepth == 0; // NOLINT
  int spaces = calculateIndent(input, &error);
  tokenLine = currentLine;

  if (spaces == -1) {
    if (error == -1) {
      goto return_error;
    } else if (error == 2) {

      handle_eof(&token);
      goto return_token;
    } else if (error == 1) {

      tokenLine = currentLine - 1; // calculateIndent() is one ahead
      token.type = TOKEN_NEWLINE;
      goto return_token;
    } else {
      fprintf(stderr,
              "Scanner error: %d, Unknown error in indent calculation\n",
              SCANNER_ERROR);
      goto return_error;
    }
  } else if (atLineStart) {
    int currentTop = *scanner_stack_top();
    int topIndex = scanner_stack_size() - 1;

    if (spaces > currentTop) {
      scanner_stack_push(spaces);
      token.type = TOKEN_INDENT;
      goto return_token;
    } else if (spaces < currentTop) {
      int index = find_number_in_stack(spaces);

      if (index == -1) {
        fprintf(stderr, "Scanner error: %d, Indentation error \n",
                SCANNER_ERROR);
        goto return_error;
      }

      int levels_to_pop = topIndex - index;

      for (int i = 0; i < levels_to_pop; i++) {
        scanner_stack_pop();
      }

      if (levels_to_pop > 1) {
        pendingDedents = levels_to_pop - 1;
      }

      token.type = TOKEN_DEDENT;
      goto return_token;
    }
  }

  while (true) {

    // realloc test
    if (len + REALLOC_CONST >= capcaity) {
      char *tmp = realloc(str, capcaity * 2);
      if (tmp == NULL) {
        fprintf(stderr, "Scanner error: %d, realloc failed\n", INTERPRET_ERROR);

        goto return_error;
      }
      str = tmp;
      capcaity *= 2;
    }

    if (state == S_START) {
      tokenLine = currentLine;
    }

    currentChar = next_char(input);

    switch (state) {

    case S_START:
      if (currentChar == EOF) {

        handle_eof(&token);
        goto return_token;
      }

      if (currentChar == '\n') {
        if (bracketDepth == 0) {
          lineStart = true;
          token.type = TOKEN_NEWLINE;
          goto return_token;
        }

        break;
      }

      if (isspace(currentChar)) {
        break;
      }

      if (currentChar == '+') {
        token.type = TOKEN_PLUS;
        goto return_token;
      }

      if (currentChar == '@') {
        token.type = TOKEN_AT;
        goto return_token;
      }

      if (currentChar == '-') {
        token.type = TOKEN_MINUS;
        goto return_token;
      }

      if (currentChar == '*') {
        token.type = TOKEN_MULTIPLY;
        goto return_token;
      }

      if (currentChar == '/') {
        int next = next_char(input);

        // One-line comment, not new line
        if (next == '/') {
          while (next != '\n' && next != EOF) {
            next = next_char(input);
          }
          put_back(next, input);
          break;
        }

        // Multi-line comment
        if (next == '*') {
          int depth = 1;
          int prev = 0;

          while (depth > 0) {
            next = next_char(input);
            if (next == EOF) {
              fprintf(stderr, "Scanner error: %d, Never ending block comment\n",
                      SCANNER_ERROR);
              goto return_error;
            }

            if (prev == '/' && next == '*') {
              depth++;
              prev = 0;
            }

            else if (prev == '*' && next == '/') {
              depth--;
              prev = 0;
            }

            else {
              prev = next;
            }
          }
          break;
        }

        // not a comment
        put_back(next, input);
        token.type = TOKEN_DIVIDE;
        goto return_token;
      }

      if (currentChar == '=') {
        int next = next_char(input);

        if (next == '=') {
          token.type = TOKEN_EQ;
          goto return_token;
        }
        put_back(next, input);
        token.type = TOKEN_ASSIGN;
        goto return_token;
      }

      if (currentChar == '>') {
        int next = next_char(input);

        if (next == '=') {
          token.type = TOKEN_GREATER_EQ;
          goto return_token;
        }
        put_back(next, input);
        token.type = TOKEN_GREAER;
        goto return_token;
      }

      if (currentChar == '<') {
        int next = next_char(input);

        if (next == '=') {
          token.type = TOKEN_LOWER_EQ;
          goto return_token;
        } else if (next == '-') {
          token.type = TOKEN_LARROW;
          goto return_token;
        }
        put_back(next, input);
        token.type = TOKEN_LOWER;
        goto return_token;
      }

      if (currentChar == '!') {
        int next = next_char(input);

        if (next == '=') {
          token.type = TOKEN_NOT_EQ;
          goto return_token;
        }
        put_back(next, input);
        token.type = TOKEN_NOT;
        goto return_token;
      }

      if (currentChar == ':') {
        token.type = TOKEN_COLON;
        goto return_token;
      }

      if (currentChar == ';') {
        token.type = TOKEN_SEMICOLON;
        goto return_token;
      }

      if (currentChar == ',') {
        token.type = TOKEN_COMMA;
        goto return_token;
      }

      if (currentChar == '(') {
        bracketDepth++;
        token.type = TOKEN_OPENING_BRACKET;
        goto return_token;
      }

      if (currentChar == ')') {
        if (bracketDepth == 0) {
          fprintf(stderr, "Scanner error %d, Invalid closing bracket\n",
                  SCANNER_ERROR);
          goto return_error;
        }
        bracketDepth--;
        token.type = TOKEN_CLOSING_BRACKET;
        goto return_token;
      }

      if (currentChar == '.') {
        int next = next_char(input);

        if (next == '.') {
          fprintf(stderr, "Scanner error: %d, invalid double dot\n",
                  SCANNER_ERROR);
          goto return_error;
        }

        else if (isdigit(next)) {
          str[len] = '.';
          len++;
          str[len] = (char)next;
          len++;

          state = S_DOUBLE;
        }

        else {
          put_back(next, input);
          token.type = TOKEN_DOT;
          goto return_token;
        }

        break;
      }

      if (currentChar == '&') {
        int next = next_char(input);

        if (next == '&') {
          token.type = TOKEN_AND;
          goto return_token;
        }
        fprintf(stderr, "Scanner error: %d, invalid char &\n", SCANNER_ERROR);
        goto return_error;
      }

      if (currentChar == '|') {
        int next = next_char(input);

        if (next == '|') {
          token.type = TOKEN_OR;
          goto return_token;
        }
        fprintf(stderr, "Scanner error: %d, invalid char |\n", SCANNER_ERROR);
        goto return_error;
      }

      if (isalpha(currentChar) || currentChar == '_') {
        state = S_ID;
        str[len] = (char)currentChar;
        len++;
        break;
      }

      if (isdigit(currentChar)) {
        state = S_INT;
        str[len] = (char)currentChar;
        len++;

        break;
      }

      if (currentChar == '"') {
        state = S_STRING;
        break;
      }

      fprintf(stderr, "Unknown character '%c' \n", currentChar);
      goto return_error;

    case S_STRING:

      if (currentChar == '\n') {
        fprintf(stderr,
                "Scanner error: %d, \\n detected in single line string\n",
                SCANNER_ERROR);

        goto return_error;
      }

      else if (currentChar == EOF) {
        fprintf(stderr, "Scanner error: %d, Invalid EOF single line string\n",
                SCANNER_ERROR);
        goto return_error;
      }

      // escape chars
      else if (currentChar == '\\') {
        // put_back not needed
        int next = next_char(input);

        if (next == 'n') {
          str[len] = '\n';
        }

        else if (next == '\\') {
          str[len] = '\\';
        }

        else if (next == '"') {
          str[len] = '\"';
        }

        else if (next == 't') {
          str[len] = '\t';
        }

        else if (next == 'r') {
          str[len] = '\r';
        }

        else {
          fprintf(stderr, "Scanner error: %d, Invalid escape char \n",
                  SCANNER_ERROR);
          goto return_error;
        }

        len++;
        continue;
      }

      // string end / move to multi-line string
      else if (currentChar == '"') {

        // third "
        int next = next_char(input);
        if (next == '"') {
          state = S_MULTI_LINE_STRING;
        } else {

          put_back(next, input);
          token.type = TOKEN_STRING;
          str[len] = '\0';
          token.value.string = strdup(str);
          goto return_token;
        }
      }

      // Write string to arr
      else {
        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_MULTI_LINE_STRING:
      if (currentChar == EOF) {
        fprintf(stderr, "Scanner error: %d, Invalid EOF in multi-line string\n",
                SCANNER_ERROR);
        goto return_error;
      }

      else if (currentChar == '"') {
        int next1 = next_char(input);
        int next2 = next_char(input);

        if (next1 == '"' && next2 == '"') {
          str[len] = '\0';
          token.type = TOKEN_STRING;
          token.value.string = strdup(str);
          goto return_token;
        }
        put_back(next2, input);
        put_back(next1, input);

        str[len] = (char)currentChar;
        len++;
      } else {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_INT:

      if (!isdigit(currentChar)) {
        // check for double __
        if (currentChar == '_') {
          int next = next_char(input);

          if (!isdigit(next)) {
            fprintf(stderr, "Scanner error: %d, double _ in int\n",
                    SCANNER_ERROR);

            goto return_error;
          }
          put_back(next, input);
        }

        else if (currentChar == 'e' || currentChar == 'E' ||
                 currentChar == '.') {
          state = S_DOUBLE;

          str[len] = (char)currentChar;
          len++;

          break;
        }

        else if (currentChar == 'd' || currentChar == 'D') {
          int next = next_char(input);

          if (!number_token_end(next)) {
            fprintf(stderr, "Scanner error: %d, invalid suffix\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          put_back(next, input);

          str[len] = '\0';
          token.type = TOKEN_DOUBLE;
          token.value.numberDouble = strtod(str, NULL);
          goto return_token;
        }

        else if (number_token_end(currentChar)) {

          put_back(currentChar, input);
          str[len] = '\0';

          // len > 1 because we want to pass "0" not "01"
          if (str[0] == '0' && len > 1) {
            fprintf(stderr,
                    "Scanner error: %d, Int cant start with leading zero\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          /* AI usage - Gemini
            AI was used here to move from atoi to strtol and explain
            difference Prompt: whats the difference between atoi and strtol,
            convert it to strtol
          */
          char *end;
          int val = (int)strtol(str, &end, 10); // 10 means base ten

          if (end == str || *end != '\0' || errno == ERANGE) {
            fprintf(stderr, "Scanner error while converting to integer\n");
            goto return_error;
          }
          //  End of AI snippet

          token.type = TOKEN_INT;
          token.value.numberInt = val;
          goto return_token;
        }

        else {
          fprintf(stderr,
                  "Scanner error: %d, invalid char detected in integer\n",
                  SCANNER_ERROR);
          goto return_error;
        }
      } else {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_DOUBLE:

      // Check for multiple E e . and invalid chars
      if (!isdigit(currentChar)) {
        if (currentChar == 'e' || currentChar == 'E' || currentChar == '.') {
          for (size_t i = 0; i < len; i++) {
            if (str[i] == 'e' || str[i] == 'E' ||
                (currentChar == '.' && str[i] == '.')) {
              fprintf(
                  stderr,
                  "Scanner error: %d, multiple exponents in double format\n",
                  SCANNER_ERROR);
              goto return_error;
            }
          }

          // e/E/. cant be next to each other
          if (!isdigit(str[len - 1])) {
            fprintf(stderr,
                    "Scanner error: %d, exponents and dot next to each other\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          str[len++] = (char)currentChar;
        }

        // +- after e/E
        else if ((currentChar == '+' || currentChar == '-') &&
                 (str[len - 1] == 'e' || str[len - 1] == 'E')) {

          str[len] = (char)currentChar;
          len++;
        }

        // Underscore between digits
        else if (currentChar == '_') {
          int next = next_char(input);
          if (!isdigit(str[len - 1]) || !isdigit(next)) {
            fprintf(stderr,
                    "Scanner error: %d, underscore is not between digits\n",
                    SCANNER_ERROR);
            goto return_error;
          }
          put_back(next, input);
        }

        // d / D suffix
        else if (currentChar == 'd' || currentChar == 'D') {
          int next = next_char(input);

          if (next != EOF && (!isdigit(str[len - 1]) || isdigit(next))) {
            fprintf(stderr, "Scanner error: %d, invalid suffix\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          put_back(next, input);

          str[len] = '\0';
          token.type = TOKEN_DOUBLE;
          token.value.numberDouble = strtod(str, NULL);
          goto return_token;
        }

        // new token
        else if (number_token_end(currentChar)) {

          // literal ends on e/E/.
          if (!isdigit(str[len - 1])) {
            fprintf(stderr, "Scanner error: %d, double ends on exponent\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          put_back(currentChar, input);
          str[len] = '\0';

          token.type = TOKEN_DOUBLE;
          token.value.numberDouble = strtod(str, NULL);
          goto return_token;
        }

        else {
          fprintf(stderr,
                  "Scanner error: %d, Random suffix/unknown error in double\n",
                  SCANNER_ERROR);
          goto return_error;
        }
      }

      // Write digit
      else {

        str[len] = (char)currentChar;
        len++;
      }

      break;

    case S_ID:
      if (isalnum(currentChar) || currentChar == '_') {

        str[len] = (char)currentChar;
        len++;
      }

      else {
        put_back(currentChar, input);
        str[len] = '\0';

        // its keyword
        int key = is_keyword(str);
        if (key != -1) {
          token.type = TOKEN_KEYWORD;
          token.value.keyword = keywordTable[key].keyword;
        }

        // its id
        else {
          if (strcmp(str, "_") == 0) {
            fprintf(stderr, "Scanner error: %d, '_' is not valid identifier\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          if (strcmp(str, "true") == 0) {
            token.type = TOKEN_BOOL;
            token.value.boolean = true;
            goto return_token;
          }
            
          if (strcmp(str, "false") == 0) {
              token.type = TOKEN_BOOL;
              token.value.boolean = false;
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
      goto return_error;
    }
  }

return_token:
  free(str);
  token.line = tokenLine;
  return token;

return_error:
  scanner_destroy();
  free(str);
  token.type = TOKEN_ERROR;
  token.line = tokenLine;
  return token;
}

/**
 * @brief Decides if string is keyword or identifer
 *
 * @param[in] str pointer to string
 * @returns index to keyword_table array if true, -1 if false
 */
static int is_keyword(char *str) {

  int size = sizeof(keywordTable) / sizeof(keywordTable[0]);

  for (int i = 0; i < size; i++) {
    if (strcmp(str, keywordTable[i].name) == 0) {
      return i;
    }
  }

  return -1;
}

/**
 * @brief Decides if character can be right behind number/double
 *
 * @param[in] character from input
 * @returns true if character can be behind token,, false if not
 */
static bool number_token_end(int currentChar) {
  int result =
      (currentChar == '+' || currentChar == '-' || currentChar == '*' ||
       currentChar == '/' || currentChar == '=' || currentChar == '<' ||
       currentChar == '>' || currentChar == '!' || currentChar == ')' ||
       currentChar == '(' || currentChar == ';' || isspace(currentChar) ||
       currentChar == ',' || currentChar == '|'|| currentChar == '&' || 
       currentChar == EOF);

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
static int calculateIndent(FILE *input, int *error) {
  *error = 0;
  if (lineStart && bracketDepth == 0) {
    int spaces = 0;
    int indentChar;

    while (true) {
      indentChar = next_char(input);

      if (indentChar == ' ') {
        spaces++;
      }

      else if (indentChar == '\t') {
        fprintf(stderr, "Scanner error: %d, tab at the start of a line\n",
                SCANNER_ERROR);

        *error = -1;
        return -1;
      }

      else if (indentChar == '\n') {
        *error = 1;
        return -1;
      }

      else if (indentChar == EOF) {
        *error = 2;
        return -1;
      }

      else if (indentChar == '/') {
        int next = next_char(input);

        // One-line comment from start of the line
        if (next == '/') {
          while (next != '\n' && next != EOF) {
            next = next_char(input);
          }

          spaces = 0;
          continue;
        }

        // Multi-line comment
        if (next == '*') {
          int depth = 1;
          int prev = 0;

          while (depth > 0) {
            next = next_char(input);
            if (next == EOF) {
              fprintf(stderr, "Scanner error: %d, Never ending block comment\n",
                      SCANNER_ERROR);
              *error = -1;
              return -1;
            }

            if (prev == '/' && next == '*') {
              depth++;
              prev = 0;
            }

            else if (prev == '*' && next == '/') {
              depth--;
              prev = 0;
            }

            else {
              prev = next;
            }
          }

          spaces = 0;
          continue;
        }

        // not a comment
        put_back(next, input);
        put_back(indentChar, input);
        lineStart = false;
        return spaces;
      }

      else {
        put_back(indentChar, input);
        lineStart = false;
        return spaces;
      }
    }
  }
  return 0;
}

/**
 * @brief Resets internal state of scanner, and frees alocated memory
 */
void scanner_destroy(void) {
  if (stackInitialized) {
    scanner_stack_free();
    stackInitialized = false;
  }

  bracketDepth = 0;
  pendingDedents = 0;
  lineStart = true;
  currentLine = 1;
  EndOfFileDetected = false;
}

/**
 * @brief Handles end of input: returns remaining dedents then EOF
 *
 * @param[out] Token to write the resulting type to
 */
static void handle_eof(Token *token) {
  int open_levels = scanner_stack_size() - 1;

  if (open_levels > 0) {
    while (scanner_stack_size() > 1) {
      scanner_stack_pop();
    }
    pendingDedents = open_levels - 1;
    EndOfFileDetected = true;
    token->type = TOKEN_DEDENT;
  } else {
    token->type = TOKEN_EOF;
    scanner_destroy();
  }
}

/**
 * @brief Wrapper to get new char from input
 *
 * @param[in, out] input stream
 * @returns char
 */
static int next_char(FILE *input) {
  int next = fgetc(input);
  if (next == '\n') {
    currentLine++;
  }
  return next;
}

/**
 * @brief Wrapper to return char to input
 * @param[out] input stream
 * @param[in, out] char
 */
static void put_back(int currentChar, FILE *input) {
  if (currentChar == '\n') {
    currentLine--;
  }
  ungetc(currentChar, input);
}