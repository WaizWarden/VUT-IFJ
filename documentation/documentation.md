<div class="title-page">

<h1 style="margin-bottom: 0.5rem;">Project documentation</h1>
<h4 style="margin-bottom: 0;">Implementation of Scalla compiler</h4>
<i>Hashtable variant</i>


<p style="margin-top: 1.3rem; margin-bottom: 0.1rem"><b>Team</b></p>
<p style="margin: 0; line-height: 1.5;">
<b> Leader: </b> xjuricl01 (25 %)<br>
xoramst00 (25 %) - xhorakj01 (25 %) - xsirmat00 (25 %)
</p>

</div>

# Syntax Analysis

The syntax analysis is a procces in which tokens, parsed by the lexical analyzer, are analysed and a AST (abstract syntax tree) is build.

## Architecture

The parsing architecture is divided into two segments:

- `parser.c` - handles programm structure using recursive descent.
- `precedence.c` - handles expressions using operator-precedence parsing. (precedence table)

## Recursive descent algoritm

This part processes the structure of programm using the method of recursive descent. Each function of parser converts specific syntactic constructions (Definitions of function, declaration of variables, prolog, cycles, conditions). In the reference of actual given token decides what rule to apply and what to expect further.

### How parser communicates with scanner

Helper functions expect some type of given token or keywords and loads next token. If the input does not meet the expected syntax, parser ends with critical error (syntax error)

### Prolog, Program and definition of functions

In the beggining of the process, parser skips any new lines, while expecting first prolog. This prolog must be exact string in form of "import ifj26.*" (if not met - error).

Then parser processes definition of function by function until it reaches End Of File. The resulting nodes are stored into the root node of AST that represents the whole programm.

#### What is processed with every function

- If the function acts as a main function (anotation `"@main"`)
- The name of the function
- Name of parameters, list of parameters and their types
- Explicit return type if there is any
- The block of function after `"="` symbol

#### How is the block of function defined

The block can be defined as a expression (`def f(): Int = 5 + 3`) or by a function call (`def g(): Int = f()`). The definition can be on the same line or be indnted.

Anotation of `"@main"` is only noted by parser into the AST. The corectness of sematics is not checked by parser.

### Block 

For blocks, there are 2 specific commands: `"INDENT"` and `"DEDENT"`.

  - `"INDENT"` is a unit of functing nesting further into the block
  - `"DEDENT"` is opposite to INDENT

Both `"INDEND"` and `"DEDENT"` are created by scanner based on the indentation of parsed input. Parser does not work with indentation itself, only with the two specified tokens.

#### Inside of block

Inside the blocks processes the commands and stores the AST nodes into the list.
Simple commands are separated by new line or semicolon
Block must contain at least one command. Empty block can be expressed by `"()"`

Supported block constructions:

- Declaration of variables `var` or `val`
- Assigning value to some declared variable
- Function call
- Return call
- Expression `if` with necceceary block of `else`
- `while` cycle
- The commands itself

Nested constructions are processed recursively.
Example: While processing a block of cycle, parser loads a condition `if` and then processes whole block `if`, but the nested block may contain other nested blocks which are processed earlier than the parent block - recursive principle.

#### How are expressions distincted, function calling

The standalone expressions are not processed by the `parser.c` itself, but are handled in `precedence.c`
The call of functions are processed by the `parser.c` itself.

If the construction starts with identifier, parser stores its name and loads next token. Depending on the next token decides further procedure.

- `"="` - simple assign into the variable
- `"("` - opening bracket indicates call of a function
- other token - expression that starts with identifier

Upon the handover of expression to the precedence analysis, parser hands over also AST node with lready loaded identifier.
Precedence analysyscan continue without repeated loading of the token.

Arguments of calling parser processes as a list of terms that are divided by commas, not as a general expressions.

## Operator-Precedence Parsing

Expression are processed by independent module `precedence.c` with a method of the precedence syntax analysys. In precedence.c is not used the recursive descent, but the decision is based on precedence table. Precedence table states relation between terminal on stack and actual input token.
Result of this analysis is a sub-tree that represents the parser expression. Its structure shows the priority of operators, asociativity, and grouping with `"()"`

### Stack and precedence symbols

The analysis uses implemented stack in `stack.c`, in the bottom is symbol `"$"`. During the precedence analysis stack stores: terminals, nonterminals that represents processed sub-part of the expression and the signs of the start.

Tokens from scanner are converted to precedence symbols. operators with similiar priority and its act are grouped together (+ and -), but the exact type of operator is stored to distinguish the difference between the elements in the group (+ and - cant be viewed as same element).

Upon searching the relation in precedence table is used the top terminal on the stack, not only the top element. Nonterminals are skipped in this search.

### Shift and reduction

Precedence table distinguishes 4 types of relations: 

| Relation | Action |
   |---|---|
   | `<` | Marks the start sign and inserts the input symbol on the stack and loads next token  |
   | `=` | Inserts input symbol on the stack and loads next token without insertion of start sign|
   | `>` | Does the reduction of the part of stack depending on the correct reduction rule. |
   | Empty | Expression is denied as syntax error. |


Upon reduction of the analysis removes elements to the sign of start. If it matches the relation, replaces them with nonterminals with its assigned AST node. Upon reduction the new input token is not loaded

Analysis ends correctly only if the input symbol is the ending `"$"` and on the stack remains only the stack bottom and one nonterminal that represents the whole parsed expression.

### Construction of the precedence table (from 6x6 to 9x9)

The base of precedence table was created using the lecture`s
table that featured 6x6 relations. This table contains these 
operations: `*` and `+`. Because operators with the same asociativity and same priority has identical lines and columns, they are not directly in the table and are grouped into two sections: (+,-) and (*,/)

Because the specification requires unary minus (u-), the 6x6 table cannot distinguish between normal and unary minus. Also there were more operations and comparators that required the expansion of the original table (like `<`, `<=`, or `==` and `!=`)

Relation operators and comparators are non-asociative. Because of this chaining like "a<b<c<d" or "a<b == c" is not permited by the specification.

The result is table containing 9 columns and 9 lines with symbols: +- ,  */ ,  u- ,  rel ,  ==!= ,  ( ,  ) ,  i ,  $

#### The conversion of 6x6 table to 9x9 itself

Reduction rules:
  - E -> i (term)
  - E -> (E)
  - E -> E +- E (for + and -)
  - E -> E */ E (for * and /)
  - E → E rel E  (for <, <=, >, >=)
  - E → E ==!= E (covers == and !=)
  - E -> -E(Unary minus)
  - E -> ( ) (unit literal)

For every reduction rule exist a sematic rule that creates AST

1) Expansion for unary minus (-u) 6x6 to 7x7
  - I had to add a new symbol into the table `-u`
  - Column for operators `(`, `$`  and the `u-` itself is `<`
    - for `i` and `(` is empty
  - The line is the same like for */
2) Expansion for rel
  - Column: for `+-`, `*/` , `u-` and `)`, `$` is the value `<`, otherwise `>`
  - Line: `+-`, `*/`, `u-` is `>` because of asociativity
3) Expansion for `==` and `!=`
  - The lowest priority is created like the `rel 
  - Other operators must be `>

### How does the expression end

The IFJ26 does not have a special ending symbol, precedence cannot determine the end until the input symbol is `$

Upon conversion of tokens to precedence symbols are on $ mapped like:
  - NEWLINE - End of an expression statement or initializer
  - SEMICOLON - Statement separator inside a block
  - DEDENT - Expression as the last statement of a block
  - EOF - Expression at the end of the input
  - `then` - End of an `if` condition
  - `do` - End of a `while` condition

The ending symbol is not consumed and is instead given back to the parser itself and stays stored in `currentToken`

Two requirements for sucessfull ending of expression:
  - Input symbol is `$`
  - The stack contains exactly `$` and one non-terminal (`E`)

If `$` arrives in the middle of construction process, it ends with an error.
