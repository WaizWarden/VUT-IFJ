/**
* @file    test_stack.c
* @author  xhorakj01
* @brief   Tests for stack
*/

#include "test_stack.h"
#include <assert.h>
#include "../src/stack.h"

/**
 * @brief Testing whenether the item successfully pushed and checking his position
 */

/*
 * Here begins a help of Devin AI agent on how to implement "test_push_and_top" function
 */

static void test_push_and_top(void) {
    StackItem item;

    stack_init();
    item.symbol = SYM_DOLLAR;
    item.token = TOKEN_EOF;
    item.node = NULL;
    item.isHandleStart = false;
    stack_push(item);

    //Getting the Top item
    StackItem *top = stack_top();

    assert(top != NULL);
    assert(top->symbol == SYM_DOLLAR);
    assert(top->token == TOKEN_EOF);
    assert(!top->isHandleStart);
    assert(top->node == NULL);

    stack_free();

}

//End of collaboration with Devin AI

/**
 * @brief Testing the correct order of elements in stack
 */
static void test_lifo_order(void) {
    stack_init();

    //Creation of 3 different Items
    StackItem item0;
    item0.symbol = SYM_DOLLAR;
    item0.token = TOKEN_EOF;
    item0.node = NULL;
    item0.isHandleStart = false;

    StackItem item1;
    item1.symbol = SYM_PLUS_MINUS;
    item1.token = TOKEN_PLUS;
    item1.node = NULL;
    item1.isHandleStart = false;

    StackItem item2;
    item2.symbol = SYM_TERM;
    item2.token = TOKEN_INT;
    item2.node = NULL;
    item2.isHandleStart = true;

    stack_push(item0);
    stack_push(item1);
    stack_push(item2);

    StackItem popped = stack_pop();

    //Checking after first popping
    assert(popped.symbol == SYM_TERM);
    assert(popped.token == TOKEN_INT);
    assert(popped.node == NULL);
    assert(popped.isHandleStart);

    popped = stack_pop();
    //Checking if he item1 is availible at the correct pos
    assert(popped.symbol == SYM_PLUS_MINUS);
    assert(popped.token == TOKEN_PLUS);
    assert(popped.node == NULL);
    assert(!popped.isHandleStart);

    popped = stack_pop();
    //Last check
    assert(popped.symbol == SYM_DOLLAR);
    assert(popped.token == TOKEN_EOF);
    assert(popped.node == NULL);
    assert(!popped.isHandleStart);

    //freeing of memory
    stack_free();
}

/**
 * @brief Testing the corectness of top terminal using elements on stack "$ + E E", the top should be '+'
 */
static void test_top_terminal(void) {
    stack_init();

    //Creation of 4 items in stack
    StackItem item0;
    item0.symbol = SYM_DOLLAR;
    item0.token = TOKEN_EOF;
    item0.node = NULL;
    item0.isHandleStart = false;

    StackItem item1;
    item1.symbol = SYM_PLUS_MINUS;
    item1.token = TOKEN_PLUS;
    item1.node = NULL;
    item1.isHandleStart = false;

    StackItem item2;
    item2.symbol = SYM_NONTERM;
    item2.token = TOKEN_EOF;
    item2.node = NULL;
    item2.isHandleStart = true;

    StackItem item3;
    item3.symbol = SYM_NONTERM;
    item3.token = TOKEN_EOF;
    item3.node = NULL;
    item3.isHandleStart = true;

    stack_push(item0);
    stack_push(item1);
    stack_push(item2);
    stack_push(item3);

    StackItem *topTerminal = stack_top_terminal();

    assert(topTerminal != NULL);
    assert(topTerminal->symbol == SYM_PLUS_MINUS);
    assert(topTerminal->token == TOKEN_PLUS);

    // Creating new fifth -> if on top is terminal, that terminal returns
    StackItem item4;
    item4.symbol = SYM_TERM;
    item4.token = TOKEN_IDENTIFIER;
    item4.node = NULL;
    item4.isHandleStart = false;
    stack_push(item4);

    topTerminal = stack_top_terminal();

    //Verification of item4 values
    assert(topTerminal != NULL);
    assert(topTerminal->symbol == SYM_TERM);
    assert(topTerminal->token == TOKEN_IDENTIFIER);

    stack_free();
}

/**
 * @brief Testing of resizing the stack (growth)
 */
static void test_stack_growth(void) {
    stack_init();

    // Setting up the AST nodes - 16x
    AstNode node[16];
    for (int i = 0; i < 16; i ++){
        StackItem item;
        item.symbol = SYM_TERM;
        item.token = TOKEN_INT;
        item.node = &node[i];
        if(i%3 == 0){
            item.isHandleStart = true;
        }else{
            item.isHandleStart = false;
        }

        stack_push(item);
    }

    // After the ninth element added in the loop above, a realloc should be triggered
    // (stack_init() sets the default capacity to 8 and after realloc it is 2*n)

    for(int i = 15; i >=0; i--){
        StackItem popped = stack_pop();

        assert(popped.symbol == SYM_TERM);
        assert(popped.token == TOKEN_INT);
        assert(popped.node == &node[i]);
        assert(popped.isHandleStart == (i % 3 == 0));
    }

    stack_free();
}

/**
 * @brief Main function of test stack.
 */
void test_stack(void) {
    test_push_and_top();
    test_lifo_order();
    test_top_terminal();
    test_stack_growth();
}
