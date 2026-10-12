/**
 * @file    stack.c
 * @author  xhorakj01
 * @brief   Stack using dynamic array
 */

#include "stack.h"
#include <stdio.h>
#include <stdlib.h>

#define INTERPRET_ERROR 99

static StackItem *items = NULL; //pole
static size_t size = 0;
static size_t capacity = 0;

/**
 * @brief Inicialization of the dynamic array
 * @param AstNodeType type
 * @returns Pointer to allocated memory.
 */

void stack_init(){
    size = 0;
    capacity = 8;
    items = malloc(capacity * sizeof(StackItem));

    if(items == NULL){
        exit(INTERPRET_ERROR);
    }
}

void stack_push(StackItem item){
    if(size == capacity){
        capacity *= 2;
        StackItem *newItems = realloc(items, capacity * sizeof(StackItem));

        if(!newItems){
            exit(INTERPRET_ERROR);
        }
        items = newItems;
    }
    items[size++] = item;
}

StackItem stack_pop(void){
       if (size == 0) {
           exit(INTERPRET_ERROR);
       }
       return items[--size];
   }

StackItem *stack_top(void){
    if (size == 0){
        exit(INTERPRET_ERROR);
    }
    return &items[size - 1];
}

/*  Bylo použito Gemini LLM pro radu s implementací následující funkce.
 *  Citace LLM: "Algoritmus se ptá tabulky na a  = terminál nejblíže vrcholu. Na zásobníku ale může být navrchu neterminál  E  (stav třeba  $E+<E ), takže musíš sestoupit od vrcholu
  dolů a přeskočit  SYM_NONTERM"
 */
StackItem *stack_top_terminal(void){
    for (size_t i = size; i > 0; i--) {
        if (items[i-1].symbol != SYM_NONTERM) {
            return &items[i-1];
        }
    }
    exit(INTERPRET_ERROR);  // $ je vždy dole, nemůže nastat
}

void stack_free(void){
       free(items);
       items = NULL;
       size = 0;
       capacity = 0;
   }
