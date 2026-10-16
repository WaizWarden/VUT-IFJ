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
 * @brief Main function of test stack.
 */
void test_stack(void) {
    test_push_and_top();
    test_lifo_order();
}
