/**
 * @file    test_scanner.c
 * @author  xjuricl01
 * @brief   Tests for scanner
 */

#include "../src/scanner.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* AI usage - Gemini
    Showcase of fmemopen()
    Prompt: I want to test my c code, from inside (so it has to be test.c),
    problem is that it needs a file on stdin and i dont want to create files
*/

// NOLINTBEGIN

/**
 * @brief Creates a virtual file containg testing data and tests scanner, prints
 * results to screen
 *
 * @param[in] testName Name of the test
 * @param[in] inputStr Input stream, lexer will be tested on it
 * @param[in] expectedTokenType Token expected from the operation
 * @param[in] expectedOutput Expected string value string means any string not
 * just string attribute (like identifer,kw), NULL if not needed
 * @param[in] expectedInt Expected int value, -1 if not needed
 * @param[in] expectedDouble Expected double value, -1 if not needed
 */
void run_test(char *testName, char *inputStr, TokenType expectedTokenType,
              char *expectedOutput, int expectedInt, double expectedDouble) {

  FILE *file = fmemopen(inputStr, strlen(inputStr), "r");
  Token token = get_token(file);
  fclose(file);

  bool tokenTypeOk = false;
  bool outputOk = false;

  if (token.type == expectedTokenType) {
    tokenTypeOk = true;
  }

  if (token.type == TOKEN_STRING || token.type == TOKEN_IDENTIFIER) {

    char *str_ptr = (token.type == TOKEN_STRING) ? token.value.string
                                                 : token.value.identifier;

    if (expectedOutput == NULL) {
      outputOk = true;
    }

    else {
      if (token.type == TOKEN_STRING) {
        if (strcmp(expectedOutput, token.value.string) == 0) {
          outputOk = true;
        }
      }

      if (token.type == TOKEN_IDENTIFIER) {
        if (strcmp(expectedOutput, token.value.identifier) == 0) {
          outputOk = true;
        }
      }
    }

    if (tokenTypeOk && outputOk) {
      printf("Test: %s Passed\n", testName);
    }

    else if (!tokenTypeOk) {
      printf("Test: %s Failed, unexpected token type: %u\n", testName,
             token.type);
    }

    else if (!outputOk) {
      printf("Test: %s Failed, expected: %s; got %s\n", testName,
             expectedOutput, token.value.string);
    }
    free(str_ptr);
  }

  else if (token.type == TOKEN_INT || token.type == TOKEN_DOUBLE) {

    if (expectedTokenType == TOKEN_INT) {
      if (expectedInt == token.value.numberInt) {
        outputOk = true;
      }
      if (expectedInt == -1) {
        outputOk = true;
      }
    }

    else {
      if (expectedDouble == token.value.numberDouble) {
        outputOk = true;
      }
      if (expectedDouble == -1) {
        outputOk = true;
      }
    }

    if (tokenTypeOk && outputOk) {
      printf("Test: %s Passed\n", testName);
    }

    else if (!tokenTypeOk) {
      printf("Test: %s Failed, unexpected token type: %u\n", testName,
             token.type);
    }

    else if (!outputOk) {
      printf("Test: %s Failed, expected: %d; got %d\n", testName, expectedInt,
             token.value.numberInt);
    }
  }

  else {
    if (tokenTypeOk) {
      printf("Test: %s Passed\n", testName);
    } else {
      printf("Test: %s Failed, unexpected token type: %u\n", testName,
             token.type);
    }
  }
}

/**
 * @brief Tests for first iteration
 */
void first_iteration(void) {
  run_test("Operator plus", "+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Operator minus", "-", TOKEN_MINUS, NULL, -1, -1);
  run_test("Operator multiply", "*", TOKEN_MULTIPLY, NULL, -1, -1);
  run_test("Operator divide", "/", TOKEN_DIVIDE, NULL, -1, -1);
  run_test("Operator assign", "=", TOKEN_ASSIGN, NULL, -1, -1);
  run_test("Colon", ":", TOKEN_COLON, NULL, -1, -1);
  run_test("Operator equal", "==", TOKEN_EQ, NULL, -1, -1);
  run_test("Operator not equal", "!=", TOKEN_NOT_EQ, NULL, -1, -1);
  run_test("Operator not", "!", TOKEN_NOT, NULL, -1, -1);
  run_test("Operator lower", "<", TOKEN_LOWER, NULL, -1, -1);
  run_test("Operator lower or equal", "<=", TOKEN_LOWER_EQ, NULL, -1, -1);
  run_test("Operator greater", ">", TOKEN_GREAER, NULL, -1, -1);
  run_test("Operator greater or equal", ">=", TOKEN_GREATER_EQ, NULL, -1, -1);
  run_test("Semicolon", ";", TOKEN_SEMICOLON, NULL, -1, -1);
  run_test("Opening bracket", "(", TOKEN_OPENING_BRACKET, NULL, -1, -1);
  run_test("Closing bracket", ")", TOKEN_CLOSING_BRACKET, NULL, -1, -1);
  run_test("Newline outside brackets", "\n", TOKEN_NEWLINE, NULL, -1, -1);
  run_test("Unmatched closing bracket error", ")", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("Basic string", "\"Hello World\"", TOKEN_STRING, "Hello World", -1,
           -1);
  run_test("Empty string", "\"\"", TOKEN_STRING, "", -1, -1);
  run_test("Backslashes", "\"\\\\\\\\\"", TOKEN_STRING, "\\\\", -1, -1);
  run_test("Escape", "\"Line1\\nLine2\\tTab\\\\Quote\\\"CR\\r\"", TOKEN_STRING,
           "Line1\nLine2\tTab\\Quote\"CR\r", -1, -1);

  printf("\n");
  run_test("Invalid backslashes", "\"Hello \\a World\"", TOKEN_ERROR, NULL, -1,
           -1);
  run_test("Invalid end of string", "\"Hello \\0 World\"", TOKEN_ERROR, NULL,
           -1, -1);
  run_test("Invalid escape \\n", "\"Hello\nWorld\"", TOKEN_ERROR, NULL, -1, -1);
  run_test("Unclosed string", "\"Hello World", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("Keyword var", "var", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword Double", "Double", TOKEN_KEYWORD, NULL, -1, -1);

  printf("\n");
  run_test("Identifer double", "double", TOKEN_IDENTIFIER, "double", -1, -1);
  run_test("Identifer Random", "RanDom", TOKEN_IDENTIFIER, "RanDom", -1, -1);
  run_test("Identifer Hello", "HELLO", TOKEN_IDENTIFIER, "HELLO", -1, -1);
  run_test("Identifier _", "_", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("Basic int", "1234", TOKEN_INT, NULL, 1234, -1);
  run_test("Int separator #1", "12_34", TOKEN_INT, NULL, 1234, -1);
  run_test("Int separator #2", "1_2_34", TOKEN_INT, NULL, 1234, -1);
  run_test("Int separator #3", "1_2_3_4", TOKEN_INT, NULL, 1234, -1);
  run_test("Zero", "0", TOKEN_INT, NULL, 0, -1);

  printf("\n");
  run_test("Invalid separator #1", "_1", TOKEN_IDENTIFIER, NULL, -1, -1);
  run_test("Invalid separator #2", "12__34", TOKEN_ERROR, NULL, -1, -1);
  run_test("Invalid Int #2", "12__34x", TOKEN_ERROR, NULL, -1, -1);
  run_test("Invalid Int #2", "1x2__34x", TOKEN_ERROR, NULL, -1, -1);
  run_test("Starting zero", "0123", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("Double simple", "3.14", TOKEN_DOUBLE, NULL, -1, 3.14);
  run_test("Zero", "0.0", TOKEN_DOUBLE, NULL, -1, 0.0);
  run_test("Double zeros", "007.50", TOKEN_DOUBLE, NULL, -1, 7.50);
  run_test("E sign", "1E-3", TOKEN_DOUBLE, NULL, -1, 0.001);
  run_test("e sign", "2.5e3", TOKEN_DOUBLE, NULL, -1, 2500.0);
  run_test("Underscore", "1_0.5", TOKEN_DOUBLE, NULL, -1, 10.5);
  run_test("d suffix", "3.14d", TOKEN_DOUBLE, NULL, -1, 3.14);
  run_test("dot plus", "1.5+2", TOKEN_DOUBLE, NULL, -1, 1.5);
  run_test("missing zero", ".45", TOKEN_DOUBLE, NULL, -1, 0.45);

  printf("\n");
  run_test("No number #1", "5.", TOKEN_ERROR, NULL, -1, -1);
  run_test("No number #2", "1e", TOKEN_ERROR, NULL, -1, -1);
  run_test("No number #3", "1e+", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double dots", "1.5.2", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double e", "1e5e3", TOKEN_ERROR, NULL, -1, -1);
  run_test("Underscore not between numbers", "1._5", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double underscore", "1.5__5", TOKEN_ERROR, NULL, -1, -1);
  run_test("Random suffix", "3.14s", TOKEN_ERROR, NULL, -1, -1);
  run_test("Dot", ".", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("Multi simple", "\"\"\"abc\"\"\"", TOKEN_STRING, "abc", -1, -1);
  run_test("Multi empty", "\"\"\"\"\"\"", TOKEN_STRING, "", -1, -1);
  run_test("Multi newline", "\"\"\"a\nb\"\"\"", TOKEN_STRING, "a\nb", -1, -1);
  run_test("Multi no esc", "\"\"\"a\\nb\"\"\"", TOKEN_STRING, "a\\nb", -1, -1);
  run_test("Multi one q", "\"\"\"a\"b\"\"\"", TOKEN_STRING, "a\"b", -1, -1);
  run_test("Multi two q", "\"\"\"a\"\"b\"\"\"", TOKEN_STRING, "a\"\"b", -1, -1);
  run_test("Multi spec", "\"\"\"H\nello w\\orld\n\"\"\"", TOKEN_STRING,
           "H\nello w\\orld\n", -1, -1);
  run_test("Multi no end", "\"\"\"abc", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  printf("Scanner tests done\n");
}
// NOLINTEND