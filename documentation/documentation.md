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

Inside call arguments, member access `arg.member` is supported (VALMEM). After every argument, a `while` loop consumes any number of `. identifier` pairs and wraps the argument into  `ast_new_member_call`, which desugars it into a builtin function call.

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

The result is table containing 9 columns and 9 lines with symbols: +- ,  */ ,  u- , !,  rel ,  ==!= , &&, || , ., ( ,  ) ,  i ,  $

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
  - E -> E && E
  - E -> E || E
  - E -> !E - negation (ast_new_unary(OP_UNARY_NOT))
  - E → E . i

For every reduction rule exist a sematic rule that creates AST

1) Expansion for unary minus (-u) 6x6 to 13x13
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
4) Expansion for `!` - unary negation
  - It is new prefix operator, with the same priority as `u-`
  - New line and column are identical to the `u-`
    - The two symbols differ only in the reduction rules
5) Expansion for `&&` - logical and
  - A new lowest priority level (so far)
  - Left-associative: `&&` vs `&&` is `>`, so `a && b && c` reduces left to right
  - Column: `<` for `(` and `$`, otherwise `>` - when `&&` arrives on the input, the sub-expression on the stack is reduced first
  - Line: `>` only for `&&`, `||`, `)` and `$`, `<` for all higher-priority symbols - the operator binds weaker than everything before it
6)
  - Another level bellow `&&`
  - Left-associative, same construction as `&&`
  - Column: `<` for `(` and `$`, otherwise `>`
  - Line: `>` only for `||`, `)` and `$`. The `&&` column is `<`, so `a && b || c` correctly parses as `(a && b) || c`
7) Expansion for `.` - extension for Valhem
  - The highest priority operator in the whole table
  - Column (`.` on the input): `<` for all operators, `(` and `$` - the dot is shifted as part of the operand being built. `>` for `.`, `)` and `i`, `(` is empty
  - Line (`.` on top of the stack): `<` only for `i`, which is shifted as the member name. `>` for everything else, `(` is empty - a method call with arguments `a.f(x)` is not handled by precedence, the parser resolves it
  - New special reduction rule:
    - The right operand MUST be a identifier
    - Identifier is copied and the node is desugared into AST_FUNC_CALL of a builtin function.
### Precedence table

|      | +- | */ | u- | !  | rel | ==!= | && | \|\| | .  | (  | )  | i  | $  |
|------|----|----|----|----|-----|------|----|----|----|----|----|----|----|
| +-   | >  | <  | <  | <  | >   | >    | >  | >  | <  | <  | >  | <  | >  |
| */   | >  | >  | <  | <  | >   | >    | >  | >  | <  | <  | >  | <  | >  |
| u-   | >  | >  | <  | <  | >   | >    | >  | >  | <  | <  | >  | <  | >  |
| !    | >  | >  | <  | <  | >   | >    | >  | >  | <  | <  | >  | <  | >  |
| rel  | <  | <  | <  | <  |     |      | >  | >  | <  | <  | >  | <  | >  |
| ==!= | <  | <  | <  | <  |     |      | >  | >  | <  | <  | >  | <  | >  |
| &&   | <  | <  | <  | <  | <   | <    | >  | >  | <  | <  | >  | <  | >  |
| \|\| | <  | <  | <  | <  | <   | <    | <  | >  | <  | <  | >  | <  | >  |
| .    | >  | >  | >  | >  | >   | >    | >  | >  | >  |    | >  | <  | >  |
| (    | <  | <  | <  | <  | <   | <    | <  | <  | <  | <  | =  | <  |    |
| )    | >  | >  | >  | >  | >   | >    | >  | >  | >  |    | >  |    | >  |
| i    | >  | >  | >  | >  | >   | >    | >  | >  | >  |    | >  |    | >  |
| $    | <  | <  | <  | <  | <   | <    | <  | <  | <  | <  |    | <  |    |

Row = the topmost terminal on the stack, column = input terminal. An empty cell means a syntax error.

### How does the expression end

The IFJ26 does not have a special ending symbol, precedence cannot determine the end until the input symbol is `$

Upon conversion of tokens to precedence symbols are on $ mapped like:
  - NEWLINE - End of an expression statement or initializer
  - SEMICOLON - Statement separator inside a block
  - DEDENT - Expression as the last statement of a block
  - EOF - Expression at the end of the input
  - `then` - End of an `if` condition
  - `do` - End of a `while` condition
  - `else` - End of a expression followed by an else branch
  - `if` - Guard condition that functions for `for` header
  - `for` - End of expression before `for`

The ending symbol is not consumed and is instead given back to the parser itself and stays stored in `currentToken`

Two requirements for sucessfull ending of expression:
  - Input symbol is `$`
  - The stack contains exactly `$` and one non-terminal (`E`)

If `$` arrives in the middle of construction process, it ends with an error.

### Stop identifiers

The issue is, that `to`, `until` and `by` are not a keywords (for example expression: `for i <- 0 to 10 by 2 do`) and scanner returns them as a plain identifiers, but the headers have a important function.

Precedence parser can distinguish terminals of expressions only with the context of token type (NEWLINE, `then`, `do`). Upon parsing "0 to 10", precedence reads `0` and then identifier `to`, normally it would result in two operators being behind themselves (`i i`), this would be a syntax error.

Solution to the problem:
To solve this issue, `precedence_parse` accepts an additional parameter stopIdents`, a NULL-terminated array of identifier names.

During the conversion of a token into the precedence symbol, every TOKEN_IDENTIFIER is first compared with these names (strcmp).

If a name matches, the token is converted to symbol `$` instead of identifier `i`, so the expression ends  like with any other terminator. The token is not consumed and stays in `currentToken` for the parser.

The parser defines two such lists inside `parse_for`:
  - `{"to", "until", NULL}` - terminates the lower bound expression 
  - `{"by", NULL}` - terminates the upper bound expression
Upon the end of the parser expression, parser inspects the identifier left in `currentToken`: `to` marks the inclusive bound, `until` the exclusive one, and `by` introduces the optional step.

Why not use keywords?
Making `to`, `until` and `by` reserved keywords was not an option, because they remain valid identifier names outside of a `for` header.

### The `for` loop
- The loop variable `id` and the arrow `<-` are handled by the parser.
- Bounds and step are parsed by `precedence_parse` using the `stopIdents` (See the section above - Stop identifiers)
- `to` = inclusive bound, `until` = exclusive bound
  - `by` sets the needed step
- Zero or more guards `if expression` follow. Each guard is a boolean expression stored in the `guards` array of the AST_FOR. 
- The header also exists in a multi-line form. After `for` a NEWLINE + INDENT may follow, the generator and each guard stays on their own line and the header closes with closing DEDENT before `do`.

### The `if` statement (expression and statemend - BOOLTHEN)

`parse_if` takes a flag `elseRequired`
  -  As a statement, `else` is optional.
  - As an expression (right side of symbol `=`, `return`, ...)
    - `else` is mandatory 
    - `else if` is desugared into a nested `if` inside the else branch.
      - No additional AST node is created

### Loop and branches bodies

The parse body `parse_body` is shared by `if`/`else` branches and the `for` body.

Accepted forms
  - NEWLINE INDENT block DEDENT
  - A single-line statement on the same line (example: (`if x then y = 1`))

## Abstract Syntax Tree (AST)

In the ast.c and ast.h a single struct is defined with the name `AstNode`. This is used by the entire compiler, not just the parser. All 19 node kinds (AST_PROGRAM, AST_FUNC_DEF,AST_BLOCK, AST_VAR_DECL, AST_ASSIGN, AST_IF, AST_WHILE, AST_FOR, AST_RETURN, AST_FUNC_CALL, AST_EXPR_STATEMENT, AST_BINARY, AST_UNARY, AST_IDENTIFIER and the five literals) share one tagged union `as`, so any subtree is passed around as a plain `AstNode *`.

Every node kind has its own constructor `ast_new_*`. Constructors take ownership of their arguments. Child nodes and heap strings are copied, so a single `ast_free` call on the root frees the entire tree using recursion.

What is also caried by each node
  - `line` - the source line number, used for error reporting
  - `uid` and `evalType` - initialized to `0`/`T_UNIT` by the arser and filled in later by semantic analysis. (uid is not changed by parser in any way.)

`ast_print` provides a debug helper of the tree to stderr.

A special constructor is `ast_new_member_call`. It does not create a new node kind but creates desugars `receiver.member` into an AST_FUNC_CALL of a builtin function (VALMEM), with the receiver as the first argument.

## Stack (precedence)

The stack is a dynamic array that is used primarily by the precedence anylysis.
The array contains of:
  - `StackItem`
    - `symbol` - the precedence symbol
    - `token` - the original token type
    - `node` - Pointer to the Ast subtree

Stack supports operations like push/pop/top plus two created for the precedence use:
  - `stack_top_terminal` - returns the topmost terminal, skipping nonterminals and handle markers
  - `stack_insert_handle_start` - inserts the `<` marker after the topmost terminal

The array starts at capacity 8 and doubles on every realloc. Allocation failures and pops from an empty stack end with `exit(99)`.

TODO:  LL gramatika + LL tabulka
