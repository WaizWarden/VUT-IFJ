/**
 * @file    test_scanner.c
 * @author  xjuricl01
 * @brief   Tests for scanner
 */

#include "../src/scanner.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* AI usage - Gemini
    Showcase of fmemopen()
    Prompt: I want to test my c code, from inside (so it has to be test.c),
    problem is that it needs a file on stdin and i dont want to create files
*/

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
 */
void run_test(char *testName, char *inputStr, TokenType expectedTokenType,
              char *expectedOutput, int expectedInt) {

  FILE *file = fmemopen(inputStr, strlen(inputStr), "r");
  Token token = get_token(file);
  fclose(file);

  bool tokenTypeOk = false;
  bool outputOk = false;

  if (token.type == expectedTokenType) {
    tokenTypeOk = true;
  }

  if (token.type == TOKEN_STRING || token.type == TOKEN_IDENTIFIER) {

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
  }

  else if (token.type == TOKEN_INT) {
    if (expectedInt == -1) {
      outputOk = true;
    }

    else {
      if (expectedInt == token.value.numberInt)
        outputOk = true;
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
  run_test("Operator plus", "+", TOKEN_PLUS, NULL, -1);
  run_test("Operator minus", "-", TOKEN_MINUS, NULL, -1);
  run_test("Operator multiply", "*", TOKEN_MULTIPLY, NULL, -1);
  run_test("Operator divide", "/", TOKEN_DIVIDE, NULL, -1);
  run_test("Operator assign", "=", TOKEN_ASSIGN, NULL, -1);

  printf("\n");
  run_test("Basic string", "\"Hello World\"", TOKEN_STRING, "Hello World", -1);
  run_test("Empty string", "\"\"", TOKEN_STRING, "", -1);
  run_test("Backslashes", "\"\\\\\\\\\"", TOKEN_STRING, "\\\\", -1);
  run_test("Escape", "\"Line1\\nLine2\\tTab\\\\Quote\\\"CR\\r\"", TOKEN_STRING,
           "Line1\nLine2\tTab\\Quote\"CR\r", -1);

  printf("\n");
  run_test("Invalid backslashes", "\"Hello \\a World\"", TOKEN_ERROR, NULL, -1);
  run_test("Invalid end of string", "\"Hello \\0 World\"", TOKEN_ERROR, NULL,
           -1);
  run_test("Invalid escape \\n", "\"Hello\nWorld\"", TOKEN_ERROR, NULL, -1);
  run_test("Unclosed string", "\"Hello World", TOKEN_ERROR, NULL, -1);

  printf("\n");
  run_test("Keyword var", "var", TOKEN_KEYWORD, NULL, -1);
  run_test("Keyword Double", "Double", TOKEN_KEYWORD, NULL, -1);

  printf("\n");
  run_test("Identifer double", "double", TOKEN_IDENTIFIER, "double", -1);
  run_test("Identifer Random", "RanDom", TOKEN_IDENTIFIER, "RanDom", -1);
  run_test("Identifer Hello", "HELLO", TOKEN_IDENTIFIER, "HELLO", -1);

  printf("\n");
  run_test("Basic int", "1234", TOKEN_INT, NULL, 1234);
  run_test("Int separator #1", "12_34", TOKEN_INT, NULL, 1234);
  run_test("Int separator #2", "1_2_34", TOKEN_INT, NULL, 1234);
  run_test("Int separator #3", "1_2_3_4", TOKEN_INT, NULL, 1234);

  printf("\n");
  run_test("Invalid separator #1", "_1", TOKEN_IDENTIFIER, NULL, -1);
  run_test("Invalid separator #2", "12__34", TOKEN_ERROR, NULL, -1);
  run_test("Starting zero #1", "0123", TOKEN_ERROR, NULL, -1);
  run_test("Starting zero #2", "0", TOKEN_ERROR, NULL, -1);

  printf("\n");
  printf("Scanner tests done\n");
}