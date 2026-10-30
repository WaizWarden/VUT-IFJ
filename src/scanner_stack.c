/**
 * @file    scanner_stack.c
 * @author  xhorakj01, xjuricl01
 * @brief   Stack using dynamic array used for indent and dedent
 */

#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

static int *items = NULL; // array
static size_t size = 0;
static size_t capacity = 0;

/**
 * @brief Inicialization of the dynamic array
 */
void scanner_stack_init()
{
    size = 0;
    capacity = 8;
    items = malloc(capacity * sizeof(int));

    if (items == NULL)
    {
        exit(INTERPRET_ERROR);
    }
}

/**
 * @brief Pushes item to stack
 * @param Item you want to add on stack
 */
void scanner_stack_push(int item)
{
    if (size == capacity)
    {
        capacity *= 2;
        int *newItems = realloc(items, capacity * sizeof(int));

        if (!newItems)
        {
            exit(INTERPRET_ERROR);
        }
        items = newItems;
    }
    items[size++] = item;
}

/**
 * @brief Returns item from top of stack and removes value
 * @param item stored on top of stack
 */
int scanner_stack_pop(void)
{
    if (size == 0)
    {
        exit(INTERPRET_ERROR);
    }
    return items[--size];
}

/**
 * @brief Returns top
 */
int *scanner_stack_top(void)
{
    if (size == 0)
    {
        exit(INTERPRET_ERROR);
    }
    return &items[size - 1];
}

void scanner_stack_free(void)
{
    free(items);
    items = NULL;
    size = 0;
    capacity = 0;
}

size_t scanner_stack_size(void) { return size; }
