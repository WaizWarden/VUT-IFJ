#include "precedence.h"
#include "hashtable.h"
#include "stack.h"
#include <stdlib.h>
#define PRECEDENCE_TERMINAL_COUNT 6

/**
 * @Precedence table from IFJ 8 lecture
 */
static const PrecedenceRelation precedenceTable[PRECEDENCE_TERMINAL_COUNT][PRECEDENCE_TERMINAL_COUNT] = {
        {PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER, PRECEDENCE_LESS, PRECEDENCE_GREATER},
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_EQUAL, PRECEDENCE_LESS, PRECEDENCE_EMPTY},
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER},
        {PRECEDENCE_GREATER, PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER, PRECEDENCE_EMPTY, PRECEDENCE_GREATER},
        {PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_LESS, PRECEDENCE_EMPTY, PRECEDENCE_LESS, PRECEDENCE_EMPTY}
};

/**
 * @brief
 * @param stackSymbol
 * @param inputSymbol
 */
PrecedenceRelation precedence_get_relation(PrecSymbol stackSymbol, PrecSymbol inputSymbol){
    //Checking the range of stackSymbol
    if(stackSymbol < SYM_PLUS_MINUS || stackSymbol >=PRECEDENCE_TERMINAL_COUNT ){
        exit(99);
    }

    //Checking the range of inputSymbol
    if (inputSymbol < SYM_PLUS_MINUS || inputSymbol >= PRECEDENCE_TERMINAL_COUNT) {
        exit(99);
    }

    return precedenceTable[stackSymbol][inputSymbol];
}

static PrecSymbol convert_token_to_symbol(TokenType tokenType){
    switch (tokenType) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return SYM_PLUS_MINUS;
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
            return SYM_MUL_DIV;
        case TOKEN_OPENING_BRACKET:
            return SYM_LPAREN;
        case TOKEN_CLOSING_BRACKET:
            return SYM_RPAREN;
        case TOKEN_INT:
        case TOKEN_DOUBLE:
        case TOKEN_IDENTIFIER:
            return SYM_TERM;
        case TOKEN_EOF:
            return SYM_DOLLAR;
        case TOKEN_ERROR:
            exit(1);
        default:
            exit(2);
    }
}
