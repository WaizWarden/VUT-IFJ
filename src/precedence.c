#include "precedence.h"
#include "ast.h"
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

static StackItem stack_item_create(Token token){
    StackItem item;
    item.symbol = convert_token_to_symbol(token.type);
    item.token = token.type;
    item.node = NULL;

    switch (token.type){
        case TOKEN_INT:
            item.node = ast_new_int(token.value.numberInt, 0);
            break;
        case TOKEN_DOUBLE:
            item.node = ast_new_double(token.value.numberDouble, 0);
            break;
        case TOKEN_IDENTIFIER:
            item.node = ast_new_identifier(token.value.identifier, 0);
            break;
        default:
            break;
    }

    return item;
}

static void precedence_simplify(void){
    StackItem handle[3];
    size_t cnt = 0;

    while(stack_top()->symbol != SYM_HANDLE_START){
        if(stack_top()->symbol == SYM_DOLLAR){
            exit(2);
        }
        if(cnt == 3){
            exit(2);
        }

        handle[cnt] = stack_pop();
        cnt++;
    }
    stack_pop();

    // Converting i to E
    if (cnt == 1 ) {
        if(handle[0].symbol == SYM_TERM){
            StackItem result;
            result.symbol = SYM_NONTERM;
            result.token = TOKEN_EOF;
            result.node = handle[0].node;

            stack_push(result);
            return;
        }
    }

    // Converting (E) to E
    if(cnt == 3){
        if(handle[0].symbol == SYM_RPAREN && handle[1].symbol == SYM_NONTERM && handle[2].symbol == SYM_LPAREN){
            StackItem result;
            result.symbol = SYM_NONTERM;
            result.token = TOKEN_EOF;
            result.node = handle[1].node;

            stack_push(result);
            return;
        }
    }

    if (cnt == 3 && handle[0].symbol == SYM_NONTERM && handle[2].symbol == SYM_NONTERM && (handle[1].symbol == SYM_PLUS_MINUS || handle[1].symbol == SYM_MUL_DIV)) {
        BinaryOperation operation;

        switch (handle[1].token) {
            case TOKEN_PLUS:
                operation = OP_ADD;
                break;
            case TOKEN_MINUS:
                operation = OP_SUB;
                break;
            case TOKEN_MULTIPLY:
                operation = OP_MUL;
                break;
            case TOKEN_DIVIDE:
                operation = OP_DIV;
                break;
            default:
                exit(99);
        }

        StackItem result;
        result.symbol = SYM_NONTERM;
        result.token = TOKEN_EOF;
        result.node = ast_new_binary(operation, handle[2].node, handle[0].node, 0);
        stack_push(result);

        return;
    }
    exit(2);
}

AstNode *precedence_parse(FILE *input){
    stack_init();

    StackItem bot;
    bot.symbol = SYM_DOLLAR;
    bot.token = TOKEN_EOF;
    bot.node = NULL;
    stack_push(bot);

    Token currentToken = get_token(input);
    PrecSymbol inputSymbol = convert_token_to_symbol(currentToken.type);

    while (1) {
        if (inputSymbol == SYM_DOLLAR && stack_size() == 2 &&stack_top()->symbol == SYM_NONTERM) {
            AstNode *result = stack_pop().node;
            stack_free();
            return result;
        }

        PrecSymbol stackSymbol = stack_top_terminal()->symbol;
        PrecedenceRelation relation = precedence_get_relation(stackSymbol, inputSymbol);

        switch (relation) {
            case PRECEDENCE_EQUAL:
                stack_push(stack_item_create(currentToken));
                currentToken = get_token(input);
                inputSymbol = convert_token_to_symbol(currentToken.type);
                break;
            case PRECEDENCE_LESS:
                stack_insert_handle_start();
                stack_push(stack_item_create(currentToken));
                currentToken = get_token(input);
                inputSymbol = convert_token_to_symbol(currentToken.type);
                break;
            case PRECEDENCE_GREATER:
                precedence_simplify();
                break;
            case PRECEDENCE_EMPTY:
                exit(2);
        }
    }
}
