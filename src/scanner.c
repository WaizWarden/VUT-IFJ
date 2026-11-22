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

typedef enum {
  INDENT_OK,
  INDENT_NEW_LINE,
  INDENT_EOF,
  INDENT_ERROR,
} IndentResult;

static int next_char(FILE *input);
static void put_back(int currentChar, FILE *input);

static int is_keyword(char *str);
static void handle_eof(Token *token);
static bool number_token_end(int currentChar);

static IndentResult calculate_indent(FILE *input, int *spaces);
static TokenType resolve_indent(int spaces);

static TokenType single_operator(FILE *input, int currentChar);
static Token scan_string(FILE *input, bool interp);
static Token scan_identifier(FILE *input, int currentChar);

static bool is_interp_state(int state);
static bool is_multiline_state(int state);

// States in scanner
typedef enum {
  S_START,
  S_INT,
  S_DOUBLE,
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
    {"Double", KW_DOUBLE},   {"Int", KW_INT},       {"String", KW_STRING},
    {"val", KW_VAL},         {"var", KW_VAR},       {"if", KW_IF},
    {"else", KW_ELSE},       {"then", KW_THEN},     {"while", KW_WHILE},
    {"do", KW_DO},           {"import", KW_IMPORT}, {"Unit", KW_UNIT},
    {"def", KW_DEF},         {"return", KW_RETURN}, {"for", KW_FOR},
    {"Boolean", KW_BOOLEAN},
};
//  End of AI usage

static ScannerStack indentStack;

static int bracketDepth = 0;
static bool stackInitialized = false;
static int pendingDedents = 0;
static int currentLine = 1;
static bool lineStart = true;
static bool EndOfFileDetected = false;

// valmem
static bool valmemReturnDot = false;

// interpolades
// AI usage - claude
// Ai pointed me in a direction how to implement nested (via stack)
typedef enum {
  enSimpleSingleLine,
  enSimpleMultiLine,
  enInterpSingleLine,
  enInterpMultiLine,
  enInterpExpression
} stackState;

static ScannerStack interpStack;

static bool interpVariable = false;
static bool interpStart = false;
static bool interpExpression = false;
static bool interpResume = false;
static bool interpEnd = false;

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

  // malloc logic
  char *str = malloc(INITIAL_STR_LEN);
  size_t capcaity = INITIAL_STR_LEN;
  size_t len = 0;
  if (str == NULL) {
    token.type = TOKEN_ERROR;
    fprintf(stderr, "Lexer error: %d, str malloc failed\n", INTERPRET_ERROR);
    goto return_token;
  }

  if (!stackInitialized) {
    scanner_stack_init(&interpStack);
    scanner_stack_init(&indentStack);
    scanner_stack_push(&indentStack, 0);
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

  if (valmemReturnDot) {
    valmemReturnDot = false;
    token.type = TOKEN_DOT;
    goto return_token;
  }

  if (interpStart) {
    interpStart = false;
    token.type = TOKEN_INTERP_START;
    goto return_token;
  }

  if (interpEnd) {
    interpEnd = false;
    token.type = TOKEN_INTERP_END;
    goto return_token;
  }

  if (interpResume) {
    token = scan_string(input, false);
    if (token.type == TOKEN_ERROR) {
      goto return_error;
    }
    goto return_token;
  }

  // line start
  if (lineStart && bracketDepth == 0) {
    int spaces = 0;
    IndentResult result = calculate_indent(input, &spaces);

    switch (result) {
    case INDENT_ERROR:
      goto return_error;

    case INDENT_EOF:
      handle_eof(&token);
      goto return_token;

    case INDENT_NEW_LINE:
      tokenLine = currentLine - 1; // calculateIndent() is one ahead
      token.type = TOKEN_NEWLINE;
      goto return_token;

    case INDENT_OK:
      TokenType type = resolve_indent(spaces);
      if (type == TOKEN_ERROR) {
        goto return_error;
      }

      if (type != TOKEN_SKIP) {
        tokenLine = currentLine;
        token.type = type;
        goto return_token;
      }
      break;
    }
  }

  while (true) {

    // realloc logic
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

      if (isalpha(currentChar) || currentChar == '_') {
        token = scan_identifier(input, currentChar);
        if (token.type == TOKEN_ERROR) {
          goto return_error;
        }
        goto return_token;
      }

      if (isdigit(currentChar)) {
        state = S_INT;
        str[len] = (char)currentChar;
        len++;
        break;
      }

      TokenType type = single_operator(input, currentChar);
      switch (type) {

      case TOKEN_ERROR:
        goto return_error;

      case TOKEN_EOF:
        handle_eof(&token);
        goto return_token;

      // internal state
      // whitespace or comment
      case TOKEN_SKIP:
        break;

      // '"' already consumed, not needed to add it back
      case TOKEN_STRING:
        token = scan_string(input, false);
        if (token.type == TOKEN_ERROR) {
          goto return_error;
        }
        goto return_token;

      case TOKEN_DOUBLE:
        str[len++] = '.'; // '.' already consumed, add it again
        state = S_DOUBLE;
        break;

      default:
        token.type = type;
        goto return_token;
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

          // valmem
          int next = next_char(input);
          if (currentChar == '.' && (isalpha(next) || next == '_')) {
            valmemReturnDot = true;

            put_back(next, input);
            str[len] = '\0';

            char *end;
            int val = (int)strtol(str, &end, 10);
            errno = 0;
            if (end == str || *end != '\0' || errno == ERANGE) {
              fprintf(stderr, "Scanner error while converting to integer\n");
              goto return_error;
            }

            token.type = TOKEN_INT;
            token.value.numberInt = val;
            goto return_token;
          }

          put_back(next, input);
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
                    "Scanner error: %d, Int canqt start with leading zero\n",
                    SCANNER_ERROR);
            goto return_error;
          }

          /* AI usage - Gemini
            AI was used here to move from atoi to strtol and explain
            difference Prompt: whats the difference between atoi and strtol,
            convert it to strtol
          */
          char *end;
          errno = 0;
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

          // valmem
          if (currentChar == '.') {
            int next = next_char(input);
            if (isalpha(next) || next == '_') {
              valmemReturnDot = true;

              str[len] = '\0';
              put_back(next, input);
              token.type = TOKEN_DOUBLE;
              token.value.numberDouble = strtod(str, NULL);
              goto return_token;
            }

            fprintf(stderr,
                    "Scanner error: %d, multiple exponents in double format\n",
                    SCANNER_ERROR);
            goto return_error;
          }

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
 * @brief Helper function that validates idetifiers, keywords, bollean
 *
 * @param[in, out] input - input to stream of chars from stdin
 * @param[out] currentChar currentChar from input (so you don't have to putback
 * before calling)
 * @return Token
 */
static Token scan_identifier(FILE *input, int currentChar) {
  Token token = {.type = TOKEN_ERROR};

  // handle malloc
  char *str = malloc(INITIAL_STR_LEN);
  size_t capacity = INITIAL_STR_LEN;
  size_t len = 0;
  if (str == NULL) {
    fprintf(stderr, "Lexer error: %d, scan_identifier malloc failed\n",
            INTERPRET_ERROR);
    return token;
  }

  // interpolades
  if (!interpVariable) { // edgecase for s"$s"
    int next = next_char(input);
    if (currentChar == 's' && next == '"') {
      free(str);
      return scan_string(input, true);
    }

    put_back(next, input);
  }

  while (isalnum(currentChar) || currentChar == '_') {

    // realloc
    if (len + REALLOC_CONST >= capacity) {
      char *tmp = realloc(str, capacity * 2);
      if (tmp == NULL) {
        fprintf(stderr, "Lexer error: %d, realloc failed\n", INTERPRET_ERROR);
        free(str);
        return token;
      }
      str = tmp;
      capacity *= 2;
    }

    str[len] = (char)currentChar;
    len++;
    currentChar = next_char(input);
  }

  put_back(currentChar, input);
  str[len] = '\0';

  // its keyword
  int key = is_keyword(str);
  if (key != -1) {
    if (interpVariable) {
      fprintf(stderr, "Scanner error: %d, keywrod cannot be interpVariable\n",
              SCANNER_ERROR);
      free(str);
      return token;
    }

    token.type = TOKEN_KEYWORD;
    token.value.keyword = keywordTable[key].keyword;
    free(str);
    return token;
  }

  // its id
  else {
    if (strcmp(str, "_") == 0) {
      fprintf(stderr, "Scanner error: %d, '_' is not valid identifier\n",
              SCANNER_ERROR);
      free(str);
      return token;
    }

    if (strcmp(str, "true") == 0) {
      if (interpVariable) {
        fprintf(stderr, "Scanner error: %d, true in interpVariable\n",
                SCANNER_ERROR);
        free(str);
        return token;
      }
      token.type = TOKEN_BOOL;
      token.value.boolean = true;
      free(str);
      return token;
    }

    if (strcmp(str, "false") == 0) {
      if (interpVariable) {
        fprintf(stderr, "Scanner error: %d, false interpVariable\n",
                SCANNER_ERROR);
        free(str);
        return token;
      }
      token.type = TOKEN_BOOL;
      token.value.boolean = false;
      free(str);
      return token;
    }

    if (interpVariable) {
      interpVariable = false;
      interpResume = true;
      interpEnd = true;
    }

    token.type = TOKEN_IDENTIFIER;
    token.value.identifier = strdup(str);
    free(str);
    return token;
  }
}

/**
 * @brief Helper function that validates single-line and multi-line comments
 *
 * @param[in, out] input - input to stream of chars from stdin
 * @param[in] currentChar currentChar from input (so you don't have to putback
 * before calling)
 * @return True - comment is valid, else otherwise
 */
static bool valid_comment(FILE *input, int currentChar) {
  int next;

  // One-line comment
  if (currentChar == '/') {
    next = next_char(input);
    while (next != '\n' && next != EOF) {
      next = next_char(input);
    }
    put_back(next, input);
    return true;
  }

  // Multi-line comment
  if (currentChar == '*') {
    int depth = 1;
    int prev = 0;

    while (depth > 0) {
      next = next_char(input);
      if (next == EOF) {
        fprintf(stderr, "Scanner error: %d, Never ending block comment\n",
                SCANNER_ERROR);
        return false;
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
    return true;
  }
  return true;
}

/**
 * @brief Helper function geos through single lines and picks TOKEN_TYPE
 * @brief Its used at start and determines if token is more complex (like
 * string)
 *
 * @param[in, out] input - input to stream of chars from stdin
 * @param[in] currentChar currentChar from input (so you don't have to putback
 * before calling)
 * @return TokenType
 */
static TokenType single_operator(FILE *input, int currentChar) {
  int next;

  switch (currentChar) {

  case '+':
    return TOKEN_PLUS;

  case '-':
    return TOKEN_MINUS;

  case '*':
    return TOKEN_MULTIPLY;

  case ':':
    return TOKEN_COLON;

  case ';':
    return TOKEN_SEMICOLON;

  case ',':
    return TOKEN_COMMA;

  case '@':
    return TOKEN_AT;

  case '}':
    if (interpExpression) {
      interpExpression = false;
      interpResume = true;
      return TOKEN_INTERP_END;
    }
    fprintf(stderr, "Scanner error: %d, Invalid interpolate character\n",
            SCANNER_ERROR);
    return TOKEN_ERROR;

  case EOF:
    return TOKEN_EOF;

  case '/':
    next = next_char(input);
    if (next == '/' || next == '*') {
      if (valid_comment(input, next)) {
        return TOKEN_SKIP;
      }
      return TOKEN_ERROR;
    }
    put_back(next, input);
    return TOKEN_DIVIDE;

  case '&':
    next = next_char(input);

    if (next == '&') {
      return TOKEN_AND;
    }

    fprintf(stderr, "Scanner error: %d, invalid char &\n", SCANNER_ERROR);
    return TOKEN_ERROR;

  case '|':
    next = next_char(input);

    if (next == '|') {
      return TOKEN_OR;
    }
    fprintf(stderr, "Scanner error: %d, invalid char |\n", SCANNER_ERROR);
    return TOKEN_ERROR;

  case '=':
    next = next_char(input);

    if (next == '=') {
      return TOKEN_EQ;
    }
    put_back(next, input);
    return TOKEN_ASSIGN;

  case '!':
    next = next_char(input);

    if (next == '=') {
      return TOKEN_NOT_EQ;
    }
    put_back(next, input);
    return TOKEN_NOT;

  case '\n':
    if (bracketDepth == 0) {
      lineStart = true;
      return TOKEN_NEWLINE;
    }
    return TOKEN_SKIP;

  case ' ':
  case '\t':
  case '\r':
    return TOKEN_SKIP;

  case '>':
    next = next_char(input);

    if (next == '=') {
      return TOKEN_GREATER_EQ;
    }
    put_back(next, input);
    return TOKEN_GREAER;

  case '<':
    next = next_char(input);

    if (next == '=') {
      return TOKEN_LOWER_EQ;
    }
    if (next == '-') {
      return TOKEN_LARROW;
    }
    put_back(next, input);
    return TOKEN_LOWER;

  case '(':
    bracketDepth++;
    return TOKEN_OPENING_BRACKET;

  case ')':
    if (bracketDepth == 0) {
      fprintf(stderr, "Scanner error %d, Invalid closing bracket\n",
              SCANNER_ERROR);
      return TOKEN_ERROR;
    }
    bracketDepth--;
    return TOKEN_CLOSING_BRACKET;

  // Special - requires you to handle it later
  case '.':
    next = next_char(input);

    if (next == '.') {
      fprintf(stderr, "Scanner error: %d, invalid double dot\n", SCANNER_ERROR);
      return TOKEN_ERROR;
    }

    if (isdigit(next)) {
      put_back(next, input);
      return TOKEN_DOUBLE;
    }
    put_back(next, input);
    return TOKEN_DOT;

  case '"':
    return TOKEN_STRING;

  default:
    fprintf(stderr, "Scanner error: %d, Unknown character '%c'\n",
            SCANNER_ERROR, currentChar);
    return TOKEN_ERROR;
  }
}

/**
 * @brief Helper function that scans through single and multi line strings
 *
 * @param[in, out] input - input to stream of chars from stdin
 * @return String token ready to be sent to parser
 */
static Token scan_string(FILE *input, bool interp) {
  Token token = {.type = TOKEN_ERROR};

  // handle malloc
  char *str = malloc(INITIAL_STR_LEN);
  size_t capacity = INITIAL_STR_LEN;
  size_t len = 0;
  if (str == NULL) {
    fprintf(stderr, "Lexer error: %d, scan_string malloc failed\n",
            INTERPRET_ERROR);
    return token;
  }

  // interpolades
  bool resume = interpResume;
  interpResume = false;

  bool multiLineString = false;
  int currentChar = next_char(input);

  // check multi-line string
  // multi-line string can start only at start not in middle
  if (!resume) {
    int next = next_char(input);
    if (currentChar == '"' && next == '"') {
      multiLineString = true;
      currentChar = next_char(input);
    } else {
      put_back(next, input);
    }

    stackState state;
    if (interp) {
      state = multiLineString ? enInterpMultiLine : enInterpSingleLine;
    } else {
      state = multiLineString ? enSimpleMultiLine : enSimpleSingleLine;
    }
    scanner_stack_push(&interpStack, state);
  }

  int top = *scanner_stack_top(&interpStack);
  bool isMultiLine = is_multiline_state(top);
  bool isInterp = is_interp_state(top);

  while (true) {

    if (currentChar == EOF) {
      fprintf(stderr, "Scanner error: %d, Invalid EOF in string_scan()\n",
              SCANNER_ERROR);
      free(str);
      return token;
    }

    if (len + REALLOC_CONST >= capacity) {
      char *tmp = realloc(str, capacity * 2);
      if (tmp == NULL) {
        fprintf(stderr, "Scanner error: %d, realloc failed\n", INTERPRET_ERROR);
        free(str);
        return token;
      }
      str = tmp;
      capacity *= 2;
    }

    if (isMultiLine) {

      if (isInterp) {
        if (currentChar == '$') {
          int next = next_char(input);

          if (next == '$') {
            str[len] = '$';
            len++;
            currentChar = next_char(input);
            continue;
          }

          // $variable
          else if (isalpha(next) || next == '_') {
            interpVariable = true;
            interpStart = true;

            put_back(next, input);
            str[len] = '\0';
            token.type = TOKEN_STRING;
            token.value.string = strdup(str);

            free(str);
            return token;
          }

          // expression inside ${
          else if (next == '{') {
            interpStart = true;
            interpExpression = true;

            str[len] = '\0';
            token.type = TOKEN_STRING;
            token.value.string = strdup(str);

            free(str);
            return token;
          }

          else {
            fprintf(stderr,
                    "Scanner error: %d, Invalid interpolate character\n",
                    SCANNER_ERROR);

            free(str);
            token.type = TOKEN_ERROR;
            return token;
          }
        }
      }

      if (currentChar == '"') {
        int next1 = next_char(input);
        int next2 = next_char(input);

        if (next1 == '"' && next2 == '"') {
          str[len] = '\0';
          token.type = TOKEN_STRING;
          token.value.string = strdup(str);
          scanner_stack_pop(&interpStack);

          free(str);
          return token;
        }
        put_back(next2, input);
        put_back(next1, input);

        str[len] = (char)currentChar;
        len++;
      } else {
        str[len] = (char)currentChar;
        len++;
      }

      // single-line string
    } else {

      if (currentChar == '\n') {
        fprintf(stderr,
                "Scanner error: %d, \\n detected in single line string\n",
                SCANNER_ERROR);

        free(str);
        return token;
      }

      if (isInterp) {
        if (currentChar == '$') {
          int next = next_char(input);

          if (next == '$') {
            str[len] = '$';
            len++;
            currentChar = next_char(input);
            continue;
          }

          // $variable
          else if (isalpha(next) || next == '_') {
            interpVariable = true;
            interpStart = true;

            put_back(next, input);
            str[len] = '\0';
            token.type = TOKEN_STRING;
            token.value.string = strdup(str);

            free(str);
            return token;
          }

          // expression inside ${
          else if (next == '{') {
            interpStart = true;
            interpExpression = true;

            str[len] = '\0';
            token.type = TOKEN_STRING;
            token.value.string = strdup(str);

            free(str);
            return token;
          }

          else {
            fprintf(stderr,
                    "Scanner error: %d, Invalid interpolate character\n",
                    SCANNER_ERROR);

            free(str);
            token.type = TOKEN_ERROR;
            return token;
          }
        }
      }

      // Single-line string can contain escape characters as two chars
      if (currentChar == '\\') {
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
          free(str);
          return token;
        }
        len++;
      }

      // end of string
      else if (currentChar == '"') {
        str[len] = '\0';
        token.type = TOKEN_STRING;
        token.value.string = strdup(str);

        free(str);
        scanner_stack_pop(&interpStack);
        return token;
      }

      // Add char to array
      else {
        str[len] = (char)currentChar;
        len++;
      }
    }
    currentChar = next_char(input);
  }
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
       currentChar == ',' || currentChar == '|' || currentChar == '&' ||
       currentChar == '}' || currentChar == EOF);

  return (bool)result;
}

/**
 * @brief Compares indentation with stack and decides if its
 * INDENT/DEDENT/ERROR
 *
 * @param[in] indentation of the current line
 * @returns TOKEN_INDENT, TOKEN_DEDENT, TOKEN_SKIP, TOKEN_ERROR
 */
static TokenType resolve_indent(int spaces) {
  int currentTop = *scanner_stack_top(&indentStack);
  int topIndex = scanner_stack_size(&indentStack) - 1;

  if (spaces > currentTop) {
    scanner_stack_push(&indentStack, spaces);
    return TOKEN_INDENT;
  } else if (spaces < currentTop) {
    int index = find_number_in_stack(&indentStack, spaces);

    if (index == -1) {
      fprintf(stderr, "Scanner error: %d, Indentation error \n", SCANNER_ERROR);
      return TOKEN_ERROR;
    }

    int levels_to_pop = topIndex - index;

    for (int i = 0; i < levels_to_pop; i++) {
      scanner_stack_pop(&indentStack);
    }

    if (levels_to_pop > 1) {
      pendingDedents = levels_to_pop - 1;
    }

    return TOKEN_DEDENT;
  } else {
    // Same level
    return TOKEN_SKIP;
  }
}

/**
 * @brief Calculates indent for control flow, functions, etc.
 *
 * @param[in, out] input stream
 * @param[out] spaces
 * @returns IndentResult
 */
static IndentResult calculate_indent(FILE *input, int *spaces) {
  *spaces = 0;
  int indentChar;

  while (true) {
    indentChar = next_char(input);

    if (indentChar == ' ') {
      *spaces += 1;
    }

    else if (indentChar == '\t') {
      fprintf(stderr, "Scanner error: %d, tab at the start of a line\n",
              SCANNER_ERROR);

      return INDENT_ERROR;
    }

    else if (indentChar == '\n') {
      return INDENT_NEW_LINE;
    }

    else if (indentChar == EOF) {
      return INDENT_EOF;
    }

    else if (indentChar == '/') {
      int next = next_char(input);

      if (next == '/' || next == '*') {
        if (!valid_comment(input, next)) {
          return INDENT_ERROR;
        }

        if (next == '/') {
          next_char(input);
        }

        *spaces = 0;
        continue;
      }

      put_back(next, input);
      put_back(indentChar, input);
      lineStart = false;
      return INDENT_OK;
    }

    else {
      put_back(indentChar, input);
      lineStart = false;
      return INDENT_OK;
    }
  }
}

/**
 * @brief Resets internal state of scanner, and frees alocated memory
 */
void scanner_destroy(void) {
  if (stackInitialized) {
    scanner_stack_free(&indentStack);
    scanner_stack_free(&interpStack);
    stackInitialized = false;
  }

  bracketDepth = 0;
  pendingDedents = 0;
  lineStart = true;
  currentLine = 1;
  EndOfFileDetected = false;

  valmemReturnDot = false;

  interpVariable = false;
  interpStart = false;
  interpResume = false;
  interpEnd = false;
  interpExpression = false;
}

/**
 * @brief Handles end of input: returns remaining dedents then EOF
 *
 * @param[out] Token to write the resulting type to
 */
static void handle_eof(Token *token) {
  int interpStackSize = scanner_stack_size(&interpStack);
  if (interpStackSize != 0) {
    scanner_destroy();
    fprintf(stderr, "Scanner error: %d, tab at the start of a line\n",
            SCANNER_ERROR);
    token->type = TOKEN_ERROR;
    return;
  }

  int open_levels = scanner_stack_size(&indentStack) - 1;

  if (open_levels > 0) {
    while (scanner_stack_size(&indentStack) > 1) {
      scanner_stack_pop(&indentStack);
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

static bool is_interp_state(int state) {
  return state == enInterpSingleLine || state == enInterpMultiLine;
}

static bool is_multiline_state(int state) {
  return state == enSimpleMultiLine || state == enInterpMultiLine;
}