/**
 * @file    parser.c
 * @author  xhorakj01
 * @brief   Implementation of parser
 */

#include "precedence.h"
#include <stdlib.h>

static void parser_advance(FILE *input, Token *currentToken){
    *currentToken = get_token(input);

    if (currentToken->type == TOKEN_ERROR) {
        exit(1);
    }
}
