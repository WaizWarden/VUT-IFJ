#include "scanner.h"
#include <stdio.h>

int main(void) {
  FILE *input = stdin;
  Token token = get_token(input);
  printf("%s", token.value.string);
}