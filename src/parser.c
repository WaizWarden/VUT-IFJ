/**
 * @file    parser.c
 * @author  xhorakj01
 * @brief   Implementation of parser
 */

#include "parser.h"
#include "precedence.h"
#include "stack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *pInput;
static Token pToken;

static void token_drop_value(void){
    if(pToken.type == TOKEN_IDENTIFIER){
        free(pToken.value.identifier);
    }
    if(pToken.type == TOKEN_STRING){
        free(pToken.value.string);
    }
}

static void advance(void){
    token_drop_value();

    pToken = get_token(pInput);
    if (pToken.type == TOKEN_ERROR){
        exit(1);
    }
}

static void expect_keyword(Keyword keyword){
    if(pToken.type == TOKEN_KEYWORD){
        if(pToken.value.keyword == keyword){
            advance();
        }else{
            exit(2);
        }
    }else{
        exit(2);
    }
}

static void expect_type(TokenType tkType){
    if(pToken.type == tkType){
        advance();
    }else{
        exit(2);
    }
}

static void check_prolog(void){
    // every file needs to have "import ifj26.*"
    expect_keyword(KW_IMPORT);
    if (pToken.type != TOKEN_IDENTIFIER){
      exit(2);
    }
    int compare = strcmp(pToken.value.identifier, "ifj26");
    if(compare != 0){
        // the part after "import" is not "ifj26"
        exit(2);
    }
    expect_type(TOKEN_DOT);
    expect_type(TOKEN_MULTIPLY);
}

AstNode *parser_parse(FILE *fileInput){
    pInput = fileInput;
    advance();

    //Checcking the presence of "import ifj26.*
    check_prolog();

    return NULL;
}


/**
 * @brief advance parser fun
 */
static void parser_advance(FILE *input, Token *currentToken) {
  *currentToken = get_token(input);

  if (currentToken->type == TOKEN_ERROR) {
    exit(1);
  }
}

AstNode *parser_parse_assignment(FILE *input, Token *currentToken) {
  if (currentToken->type == TOKEN_ERROR) {
    exit(1);
  }

  if (currentToken->type != TOKEN_IDENTIFIER) {
    exit(2);
  }

  char *character = currentToken->value.identifier;
  parser_advance(input, currentToken);

  if (currentToken->type != TOKEN_ASSIGN) {
    free(character);
    exit(2);
  }

  parser_advance(input, currentToken);
  AstNode *value = precedence_parse(input, currentToken, NULL);

  if (value == NULL) {
    free(character);
    exit(2);
  }

  return ast_new_assign(character, value, 0);
}
