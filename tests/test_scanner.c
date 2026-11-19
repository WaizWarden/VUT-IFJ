/**
 * @file    test_scanner.c
 * @author  xjuricl01
 * @brief   Tests for scanner
 */

#include "../src/scanner.h"
#include "../src/scanner_stack.h"
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
int totalTests = 0;
int passedTests = 0;

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
  totalTests++;

  FILE *file = fmemopen(inputStr, strlen(inputStr), "r");
  Token token = get_token(file);
  fclose(file);
  scanner_destroy();

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
      passedTests++;
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
      passedTests++;
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
      passedTests++;
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
  run_test("Newline outside brackets", "\n", TOKEN_NEWLINE, NULL, -1, -1);
  run_test("Unmatched closing bracket error", ")", TOKEN_ERROR, NULL, -1, -1);
  run_test("Dot", ".", TOKEN_DOT, NULL, -1, -1);
  run_test("@", "@", TOKEN_AT, NULL, -1, -1);
  run_test(",", ",", TOKEN_COMMA, NULL, -1, -1);

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
  run_test("Keyword if", "if", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword else", "else", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword then", "then", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword do", "do", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword while", "while", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Keyword import", "import", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Unit", "Unit", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Def", "def", TOKEN_KEYWORD, NULL, -1, -1);
  run_test("Uni", "return", TOKEN_KEYWORD, NULL, -1, -1);

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
  run_test("15d double", "15d", TOKEN_DOUBLE, NULL, -1, 15.0);
  run_test("missing zero", ".45", TOKEN_DOUBLE, NULL, -1, 0.45);
  run_test("double dot", "..45", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  run_test("No number #1", "5.", TOKEN_ERROR, NULL, -1, -1);
  run_test("No number #2", "1e", TOKEN_ERROR, NULL, -1, -1);
  run_test("No number #3", "1e+", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double dots", "1.5.2", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double e", "1e5e3", TOKEN_ERROR, NULL, -1, -1);
  run_test("Double underscore", "1.5__5", TOKEN_ERROR, NULL, -1, -1);
  run_test("Random suffix", "3.14s", TOKEN_ERROR, NULL, -1, -1);

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
  run_test("One-line comment Hello", "// Hello \n+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Empty one-line comment", "//\n+", TOKEN_PLUS, NULL, -1, -1);
  run_test("One-line comment EOF", "//", TOKEN_EOF, NULL, -1, -1);

  run_test("multi-line comment Hello", "/*Hello*/+", TOKEN_PLUS, NULL, -1, -1);
  run_test("multi-line contains \\n Hello", "/*\n\nHel\nlo*/+", TOKEN_PLUS,
           NULL, -1, -1);
  run_test("Empty multi-line comment", "/**/+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Multi-line comment error 1", "/**/", TOKEN_EOF, NULL, -1, -1);
  run_test("Multi-line comment error 2", "/**/\n+", TOKEN_NEWLINE, NULL, -1,
           -1);
  run_test("Multi-line comment error 3", "/*", TOKEN_ERROR, NULL, -1, -1);
  run_test("Multi-line comment error 4", "/*", TOKEN_ERROR, NULL, -1, -1);
  run_test("Multi-line comment error 5", "/* *", TOKEN_ERROR, NULL, -1, -1);
  run_test("Multi-line comment error 6", "/*/", TOKEN_ERROR, NULL, -1, -1);
  run_test("Multi-line comment error 7", "/* /", TOKEN_ERROR, NULL, -1, -1);

  run_test("Nested comment", "/* a /* b */ c */+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Inline comment in nested", "/* a /* b\\\\ \n */ c */+", TOKEN_PLUS,
           NULL, -1, -1);
  run_test("Nested empty inside", "/*/**/*/+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Nested with spaces", "/* /**/ */+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Nested 3 levels", "/*/*/**/*/*/+", TOKEN_PLUS, NULL, -1, -1);
  run_test("Comment divide", "/+", TOKEN_DIVIDE, NULL, -1, -1);
  run_test("Nested error 1", "/* /* */", TOKEN_ERROR, NULL, -1, -1);
  run_test("Nested error 2", "/*/*/", TOKEN_ERROR, NULL, -1, -1);
  run_test("Nested error 3", "/*/*/*/", TOKEN_ERROR, NULL, -1, -1);

  printf("\n");
  printf("Scanner tests done\n");
  printf("Test count: %d\t Tests passed %d\n", totalTests, passedTests);
}

/* AI USAGE - Gemini
  I need to test whole sequence of tokens for indentation and dedentation, can
  you help create it? Ai was also used to help me create tests
*/

/**
 * @brief Tests a sequence of tokens from input string
 * @param[in] testName - Name of the test
 * @param[in] inputStr - Source code to tokenize
 * @param[in] expectedTypes - Array of expected token types, in order
 * @param[in] expectedCount - Number of items in expectedTypes
 */
void run_sequence_test(char *testName, char *inputStr, TokenType *expectedTypes,
                       int expectedCount) {
  totalTests++;

  FILE *file = fmemopen(inputStr, strlen(inputStr), "r");
  bool failed = false;

  for (int i = 0; i < expectedCount; i++) {
    Token token = get_token(file);

    if (token.type == TOKEN_STRING) {
      free(token.value.string);
    } else if (token.type == TOKEN_IDENTIFIER) {
      free(token.value.identifier);
    }

    if (token.type != expectedTypes[i]) {
      printf("Test: %s Failed at token index %d, expected type: %u, got: %u\n",
             testName, i, expectedTypes[i], token.type);
      failed = true;
      break;
    }
  }

  fclose(file);

  if (!failed) {
    printf("Test: %s Passed\n", testName);
    passedTests++;
  }
}

void indentation_flow_tests(void) {
  printf("\nIndent & Dedent Tests\n");

  TokenType expectedBr[] = {TOKEN_OPENING_BRACKET, TOKEN_CLOSING_BRACKET};
  run_sequence_test("Closing bracket", "()", expectedBr, 2);
  scanner_destroy();

  // Test 1: Basic test
  // Code:
  // var x = 1
  //     var y = 2
  // x = 3
  char *code1 = "var x = 1\n    var y = 2\nx = 3\n";
  TokenType expected1[] = {TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,        TOKEN_NEWLINE,    TOKEN_INDENT,
                           TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,        TOKEN_NEWLINE,    TOKEN_DEDENT,
                           TOKEN_IDENTIFIER, TOKEN_ASSIGN,     TOKEN_INT,
                           TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("1. Indent & Dedent basic flow", code1, expected1,
                    sizeof(expected1) / sizeof(expected1[0]));

  // Test 2, Invalid Indentation
  scanner_destroy();
  char *code2 = "var x = 1\n    var y = 2\n  var z = 3\n";
  TokenType expected2[] = {TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,     TOKEN_NEWLINE,    TOKEN_INDENT,
                           TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,     TOKEN_NEWLINE,    TOKEN_ERROR};
  run_sequence_test("2. Invalid indentation error", code2, expected2,
                    sizeof(expected2) / sizeof(expected2[0]));

  // Test 3: No Indednt, no Dedent
  scanner_destroy();
  char *code3 = "var x = 1\nvar y = 2\n";
  TokenType expected3[] = {TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,        TOKEN_NEWLINE,    TOKEN_KEYWORD,
                           TOKEN_IDENTIFIER, TOKEN_ASSIGN,     TOKEN_INT,
                           TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("Same indentation level", code3, expected3,
                    sizeof(expected3) / sizeof(expected3[0]));

  // Test 4: Double Indent, Double dedent
  // if x
  //     if y
  //         z
  // w
  scanner_destroy();
  char *code4 = "if x\n    if y\n        z\nw\n";
  TokenType expected4[] = {TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_NEWLINE,
                           TOKEN_INDENT,     TOKEN_KEYWORD,    TOKEN_IDENTIFIER,
                           TOKEN_NEWLINE,    TOKEN_INDENT,     TOKEN_IDENTIFIER,
                           TOKEN_NEWLINE,    TOKEN_DEDENT,     TOKEN_DEDENT,
                           TOKEN_IDENTIFIER, TOKEN_NEWLINE,    TOKEN_EOF};

  run_sequence_test("4. Multiple dedent at once", code4, expected4,
                    sizeof(expected4) / sizeof(expected4[0]));

  // Test 5:
  // if a
  //     if b
  //         c
  //     d
  scanner_destroy();
  char *code5 = "if a\n    if b\n        c\n    d\n";
  TokenType expected5[] = {TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_NEWLINE,
                           TOKEN_INDENT,  TOKEN_KEYWORD,    TOKEN_IDENTIFIER,
                           TOKEN_NEWLINE, TOKEN_INDENT,     TOKEN_IDENTIFIER,
                           TOKEN_NEWLINE, TOKEN_DEDENT,     TOKEN_IDENTIFIER,
                           TOKEN_NEWLINE, TOKEN_DEDENT,     TOKEN_EOF};
  run_sequence_test("5. Remaining dedents after EOF", code5, expected5,
                    sizeof(expected5) / sizeof(expected5[0]));

  // Test 6: Empty Line
  scanner_destroy();
  char *code6 = "var x = 1\n\nvar y = 2\n";
  TokenType expected6[] = {TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,     TOKEN_NEWLINE,    TOKEN_NEWLINE,
                           TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,     TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("6. Blank line", code6, expected6,
                    sizeof(expected6) / sizeof(expected6[0]));

  // Test 7: Comment cannot generate indent
  // var x = 1
  //    // comment
  // var y = 2
  scanner_destroy();
  char *code7 = "var x = 1\n    // comment\nvar y = 2\n";
  TokenType expected7[] = {TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,        TOKEN_NEWLINE,    TOKEN_KEYWORD,
                           TOKEN_IDENTIFIER, TOKEN_ASSIGN,     TOKEN_INT,
                           TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("7. Comment-only indented line", code7, expected7,
                    sizeof(expected7) / sizeof(expected7[0]));

  // Test 8: Tab error
  // var x = 1
  //  var y = 2 (space here is tab)
  scanner_destroy();
  char *code8 = "var x = 1\n\tvar y = 2\n";
  TokenType expected8[] = {TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                           TOKEN_INT,     TOKEN_NEWLINE,    TOKEN_ERROR};
  run_sequence_test("8. Tab at line start", code8, expected8,
                    sizeof(expected8) / sizeof(expected8[0]));

  // Test 9: Indents in brackets are ignored
  scanner_destroy();
  // var x = (1 +
  //   2) + 3
  char *code9 = "var x = (1 +\n  2) + 3\n";
  TokenType expected9[] = {
      TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_ASSIGN,  TOKEN_OPENING_BRACKET,
      TOKEN_INT,     TOKEN_PLUS,       TOKEN_INT,     TOKEN_CLOSING_BRACKET,
      TOKEN_PLUS,    TOKEN_INT,        TOKEN_NEWLINE, TOKEN_EOF};
  run_sequence_test("9. Indent inside brackets", code9, expected9,
                    sizeof(expected9) / sizeof(expected9[0]));

  // Test 10: import ifj26.*
  scanner_destroy();
  char *code10 = "import ifj26.*\n";
  TokenType expected10[] = {TOKEN_KEYWORD,  TOKEN_IDENTIFIER, TOKEN_DOT,
                            TOKEN_MULTIPLY, TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("10. Prolog", code10, expected10,
                    sizeof(expected10) / sizeof(expected10[0]));

  // Small function
  scanner_destroy();
  char *code11 = "def f(x: Int): Int = x\n";
  TokenType expected11[] = {
      TOKEN_KEYWORD, TOKEN_IDENTIFIER, TOKEN_OPENING_BRACKET, TOKEN_IDENTIFIER,
      TOKEN_COLON,   TOKEN_KEYWORD,    TOKEN_CLOSING_BRACKET, TOKEN_COLON,
      TOKEN_KEYWORD, TOKEN_ASSIGN,     TOKEN_IDENTIFIER,      TOKEN_NEWLINE,
      TOKEN_EOF};
  run_sequence_test("12. Small function", code11, expected11,
                    sizeof(expected11) / sizeof(expected11[0]));

  //  Multi-line function
  scanner_destroy();
  char *code12 = "def f(x: Int): Int =\n    x\ndef g(): Int = 2\n";
  TokenType expected12[] = {TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_IDENTIFIER,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_IDENTIFIER,
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_EOF};
  run_sequence_test("12. Multi-line function", code12, expected12,
                    sizeof(expected12) / sizeof(expected12[0]));

  // Test 14: @main
  scanner_destroy();
  char *code13 = "@main def run(): Unit = ()\n";
  TokenType expected13[] = {TOKEN_AT,
                            TOKEN_IDENTIFIER,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_NEWLINE,
                            TOKEN_EOF};
  run_sequence_test("13. @main function", code13, expected13,
                    sizeof(expected13) / sizeof(expected13[0]));

  scanner_destroy();
  char *code14 = "def f(n: Int): Int =\n"
                 "    var r = 1\n"
                 "    while n > 1 do\n"
                 "        r = r * n\n"
                 "    r\n"
                 "def g(): Int = 0\n";
  TokenType expected14[] = {TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_IDENTIFIER,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_ASSIGN,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_GREAER,
                            TOKEN_INT,
                            TOKEN_KEYWORD,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_IDENTIFIER,
                            TOKEN_ASSIGN,
                            TOKEN_IDENTIFIER,
                            TOKEN_MULTIPLY,
                            TOKEN_IDENTIFIER,
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_IDENTIFIER,
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_EOF};
  run_sequence_test("16. While inside function", code14, expected14,
                    sizeof(expected14) / sizeof(expected14[0]));

  // Test 15: if/else in function
  scanner_destroy();
  char *code15 = "def f(n: Int): Int =\n"
                 "    if n < 2 then\n"
                 "        1\n"
                 "    else\n"
                 "        2\n"
                 "def g(): Int = 0\n";
  TokenType expected15[] = {TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_IDENTIFIER,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_LOWER,
                            TOKEN_INT,
                            TOKEN_KEYWORD,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_KEYWORD,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_DEDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_INT,
                            TOKEN_NEWLINE,
                            TOKEN_EOF};
  run_sequence_test("15. If/else inside function", code15, expected15,
                    sizeof(expected15) / sizeof(expected15[0]));

  // Test 16: Invalid dedent (2 doesnt exist)
  scanner_destroy();
  char *code16 = "def f(): Int =\n    var x = 1\n  x\n";
  TokenType expected16[] = {TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_ASSIGN,
                            TOKEN_INT, // var x = 1
                            TOKEN_NEWLINE,
                            TOKEN_ERROR};
  run_sequence_test("16. Invalid dedent in function", code16, expected16,
                    sizeof(expected16) / sizeof(expected16[0]));

  // Test 17 built in function
  scanner_destroy();
  char *code17 = "def f(): Unit =\n"
                 "    print(\"a\")\n"
                 "    val x = readInt()\n"
                 "def g(): Unit = ()\n";
  TokenType expected17[] = {TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_NEWLINE,
                            TOKEN_INDENT,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_STRING,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_NEWLINE,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_ASSIGN,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET, // val x = readInt()
                            TOKEN_NEWLINE,
                            TOKEN_DEDENT,
                            TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_COLON,
                            TOKEN_KEYWORD,
                            TOKEN_ASSIGN,
                            TOKEN_OPENING_BRACKET,
                            TOKEN_CLOSING_BRACKET,
                            TOKEN_NEWLINE,
                            TOKEN_EOF};
  run_sequence_test("17. Builtin functions", code17, expected17,
                    sizeof(expected17) / sizeof(expected17[0]));

  // Test 18: semicolon
  scanner_destroy();
  char *code18 = "val x = 1; val y = 2\n";
  TokenType expected18[] = {TOKEN_KEYWORD,    TOKEN_IDENTIFIER, TOKEN_ASSIGN,
                            TOKEN_INT,        TOKEN_SEMICOLON,  TOKEN_KEYWORD,
                            TOKEN_IDENTIFIER, TOKEN_ASSIGN,     TOKEN_INT,
                            TOKEN_NEWLINE,    TOKEN_EOF};
  run_sequence_test("22. Semicolon separator", code18, expected18,
                    sizeof(expected18) / sizeof(expected18[0]));

// END OF AI USAGE

// Valmem Tests
  scanner_destroy();
  char *valmem1 = "15.48.toInt";
  TokenType expectedValmem1[] = {TOKEN_DOUBLE, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EOF};
  run_sequence_test("1. double.toInt", valmem1, expectedValmem1,
                    sizeof(expectedValmem1) / sizeof(expectedValmem1[0]));

  scanner_destroy();
  char *valmem2 = "15.toString";
  TokenType expectedValmem2[] = {TOKEN_INT, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EOF};
  run_sequence_test("2. int.tostring", valmem2, expectedValmem2,
                    sizeof(expectedValmem2) / sizeof(expectedValmem2[0]));

  scanner_destroy();
  char *valmem3 = "\"string\".toString";
  TokenType expectedValmem3[] = {TOKEN_STRING, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EOF};
  run_sequence_test("3. string.tostring", valmem3, expectedValmem3,
                    sizeof(expectedValmem3) / sizeof(expectedValmem3[0]));

  scanner_destroy();
  char *valmem4 = "\"abc\".length";
  TokenType expectedValmem4[] = {TOKEN_STRING, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EOF};
  run_sequence_test("4. string.length", valmem4, expectedValmem4,
                    sizeof(expectedValmem4) / sizeof(expectedValmem4[0]));

  scanner_destroy();
  char *valmem5 = "15.3.floor.int";
  TokenType expectedValmem5[] = {TOKEN_DOUBLE, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_DOT, TOKEN_IDENTIFIER, TOKEN_EOF};
  run_sequence_test("5. double.floor.int", valmem5, expectedValmem5,
                    sizeof(expectedValmem5) / sizeof(expectedValmem5[0]));

}


// NOLINTEND