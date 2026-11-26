<div class="title-page">

<h1 style="margin-bottom: 0.5rem;">Project documentation</h1>
<h4 style="margin-bottom: 0;">Implementation of Scalla compiler</h4>
<i>Hashtable variant</i>


<p style="margin-top: 1.3rem; margin-bottom: 0.1rem"><b>Team</b></p>
<p style="margin: 0; line-height: 1.5;">
<b> Leader: </b> xjuricl01 (33 %)<br>
xoramst00 (33 %) - xhorakj01 (34 %) - xsirmat00 (00 %)
</p>

<p style="margin-top: 1.3rem; margin-bottom: 0.1rem"><b>Extensions</b></p>
<p style="margin: 0; line-height: 1.5;">
BOOLTHEN - CONTROL - VALMEM - INTERPOLATEDS
</p>

</div>
# Team distribution
- **xjuricl01** – project coordination, lexical analysis, fixes in semantic analysis
- **xoramst00** – symbol table, code generator (main developer)
- **xhorakj01** – parser, AST, precedence analysis, stack
- **xsirmat00** – semantic analysis, code generator (secondary developer), dropped out

#### Explanation of percent distribution
**xsirmat00** dropped out and will retake the course; per his agreement with the team he receives 0 %.

# Team collaboration
- Each module has its own unit tests
- Whole compiler is tested by e2e tests
- Before merging to main we used valgrind to check for memory leaks
- We used `clang-tidy` for extra warnings detection
- For formating we used `clang-format`
- We didn't use PR requests


# Block diagram

# FSM of lexical analysis

# Syntax Analysis

## Architecture

Scanner produces tokens, which are then processed by syntax analysis, output is the AST tree.

Structure of programm is processed by the `parser.c` using the recursive descent. Expressions are managed in `precedence.c` using precedence analysis.

The product of syntax analysis is `AST_PROGRAM` and syntax errro ends with error 2.

## Recursive descent algorithm

Each parser function handles one syntactic construct (function definitions, variable declarations, the prolog, loops, conditions). Based on the current token it decides which rule to apply and what to expect next. 

The product are nodes of AST tree, if the syntax is incorrect, programm ends with exit code 2.

### Prolog, Program and definition of functions

Parser skips any leading newlines and expects the prolog. The prolog should be exactly `import ifj26.*`.

Prolog is mandatory exactly once in the beginning of programm.

The parser then processes function definitions one by one until it reaches EOF. The resulting nodes are stored in the root AST node (representing programm).

#### How is the function body defined

The body can be a single expression (`def f(): Int = 5 + 3`), a function call (`def g(): Int = f()`), or an indented block. The definition can be on the same line or on indented lines below.

The `@main` annotation is only recorded in the AST by the parser; its semantic correctness is not checked here.

### Block 

Scanner generates 2 tokens (indenging).

  - `INDENT` marks nesting one level deeper into a block
  - `DEDENT` marks leaving the block

Parser works with only these 2 tokens, errors of indentation are handled by the scanner.

#### Inside of block

Inside a block, the parser processes statements.
Simple statements are separated by a newline or a semicolon.
A block must not be empty.

#### Distinguishing expressions and function calls

Standalone expressions are not handled by `parser.c` itself but are passed to `precedence.c`.
Function calls are parsed by `parser.c` directly.

This is the result of absence of expression FUNEXP.

If a statement starts with an identifier, the parser stores its name and looks at the next token to decide:

- `=` - an assignment to the variable
- `(` - a function call
- anything else - an expression starting with that identifier

When handing the expression over to precedence analysis, the parser also passes the AST node for the already-read identifier, so the precedence parser can continue without re-reading it.

Call arguments are parsed as a comma-separated list of terms, not as general expressions.

Inside call arguments, member access `arg.member` is supported (VALMEM).

## Operator-Precedence Parsing

Module `precedence.c` is independant from `parser.c`
A bottom-up operator-precedence method is used and decides using the precedence table. The result is Ast subtree, the structure matches priority, asociativity and parens.

### Stack and precedence symbols

The analysis uses implemented stack in `stack.c`, in the bottom is symbol `"$"`. The stack includes precedence terminals, nonterminal `E` and handle-start markers. 

Operators with the same priority and asociativity shares lines/columns in the table. 

Original token is stored for the correct creation of the AST node ( +  vs  - ,  *  vs  / ).

Upon searching the relation in precedence table the top terminal is used on the stack, not only the top element. Nonterminals are skipped in this search.

### Shift and reduction

Reduction rules:
  - E → i 
  - E → (E) 
  - E → E op E 
  - E → u-E 
  - E → !E 
  - E → E . id 
  - E → ()

Precedence table distinguishes 4 types of relations: 

| Relation | Action |
   |---|---|
   | `<` | Inserts a handle-start marker `<`, shifts the input symbol and loads the next token |
   | `=` | Inserts input symbol on the stack and loads next token without insertion of start sign|
   | `>` | Does the reduction of the part of stack depending on the correct reduction rule. |
   | Empty | Syntax error. |

On reduction the analysis pops elements up to the `<` marker. If they match a reduction rule, they are replaced by one nonterminal carrying the new AST node. No new input token is loaded during a reduction.

Reduction links AST subtrees into new nodes.

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

Upon conversion of tokens to precedence symbols are on $ mapped like:
  - NEWLINE - End of an expression statement or initializer
  - SEMICOLON - Statement separator inside a block
  - DEDENT - Expression as the last statement of a block
  - EOF - Expression at the end of the input
  - `then` - End of an `if` condition
  - `do` - End of a `while` condition
  - `else` - End of a expression followed by an else branch
  - `if` - End of a guard expression in a `for` header
  - `for` - End of expression before `for`

The ending symbol is not consumed and is instead given back to the parser itself and stays stored in `currentToken`

Two requirements for successful ending of expression:
  - Input symbol is `$`
  - The stack contains exactly `$` and one non-terminal (`E`)

### Stop identifiers

The issue is, that `to`, `until` and `by` are not a keywords (for example expression: `for i <- 0 to 10 by 2 do`) and scanner returns them as a plain identifiers, but the headers have a important function.

To solve this issue, `precedence_parse` accepts an an additional parameter `stopIdents`, a NULL-terminated array of identifier names.

If a name matches, the token is converted to symbol `$` instead of identifier `i`, so the expression ends  like with any other terminator. The token is not consumed and stays in `currentToken` for the parser.

The parser defines two such lists inside `parse_for`:
  - `{"to", "until", NULL}` - terminates the lower bound expression 
  - `{"by", NULL}` - terminates the upper bound expression
Upon the end of the parser expression, parser inspects the identifier left in `currentToken`: `to` marks the inclusive bound, `until` the exclusive one, and `by` introduces the optional step.

### The `if` statement and `else` (expression and statement - BOOLTHEN)

`parse_if` takes a flag `elseRequired`
  -  As a statement, `else` is optional.
  - As an expression (right side of symbol `=`, `return`, ...)
    - `else` is mandatory 
    - `else if` is desugared into a nested `if` inside the else branch.
      - No additional AST node is created

CONTROL allows single-line bodies and indented blocks.

## Abstract Syntax Tree (AST)

In the ast.c and ast.h a single struct is defined with the name `AstNode`. This is used by the entire compiler, not just the parser. All 19 node kinds (AST_PROGRAM, AST_FUNC_DEF, ... , AST_IDENTIFIER and the five literals) share one tagged union `as`, so any subtree is passed around as a plain `AstNode *`.

Every node kind has its own constructor `ast_new_*`. Constructors take ownership of their arguments. Ownership of child nodes and heap strings is transferred, so a single `ast_free` call on the root frees the entire tree using recursion.

What is also carried by each node
  - `uid` and `evalType` - initialized to `0`/`T_UNIT` by the parser and filled in later by semantic analysis

A special constructor is `ast_new_member_call`. It does not create a new node kind but desugars `receiver.member` into an AST_FUNC_CALL of a builtin function (VALMEM), with the receiver as the first argument.

Mapping of valhem:

 - .toString  →  toStr 
 - .toInt  →  toInt 
 - .floor  → interní  $floor 
 - .length  →  strLength
 - unknown -> $valmem_unknown

 Interpolateds are represented by a chaining + operation.
 
## Stack (precedence)

The stack is a dynamic array that is used primarily by the precedence analysis.
The array consists of:
  - `StackItem`
    - `symbol` - the precedence symbol
    - `token` - the original token type
    - `node` - Pointer to the Ast subtree

Stack supports operations like push/pop/top plus two created for the precedence use:
  - `stack_top_terminal` - returns the topmost terminal, skipping nonterminals and handle markers
  - `stack_insert_handle_start` - inserts the `<` marker after the topmost terminal

## LL Grammar

The parser is implemented using a recursive descent that uses an LL grammar covering the program structure.
Non terminals are written using brackets `<>`. Terminals are written as tokens (`NEWLINE, INDENT, DEDENT,..`)
Expressions themselves are handled by the precedence parser and appear as a single terminal `<expr>`.

Precedence grammar for `<expr>` (not part of the LL grammar):

```
E → i | E +- E | E */ E | u-E | !E | E rel E | E ==!= E
  | E && E | E || E | E . i | ( E ) | ( )
```

### Numbered rules

```
1  <program>    → <prolog> NEWLINE <funcList>
2  <prolog>     → import ifj26 . *
3  <funcList>   → <funcDef> <funcList>
4  <funcList>   → ε
5  <funcDef>    → [@ main] def id ( <paramList> ) [: <type>] = <funcBody>
6  <funcBody>   → NEWLINE INDENT <block> DEDENT
7  <funcBody>   → <exprOrCall>
8  <paramList>  → <param> <paramRest>
9  <paramList>  → ε
10 <paramRest>  → , <param> <paramRest>
11 <paramRest>  → ε
12 <param>      → id : <type>
13 <type>       → Int | Double | String | Unit | Boolean
14 <block>      → <stmtSeq>
15 <stmtSeq>    → <statement> <sep> <stmtSeq>
16 <stmtSeq>    → <statement>
17 <sep>        → NEWLINE
18 <sep>        → ;
19 <statement>  → <varDecl>
20 <statement>  → <ifStmt>
21 <statement>  → <while>
22 <statement>  → <for>
23 <statement>  → <return>
24 <statement>  → <idStmt>
25 <statement>  → <exprStmt>
26 <varDecl>    → (var|val) id [: <type>] = <exprOrCall>
27 <idStmt>     → id = <exprOrCall>
28 <idStmt>     → id ( <argList> )
29 <idStmt>     → id <exprTail>          *
30 <return>     → return <exprOrCall>
31 <return>     → return                  *
32 <exprStmt>   → <expr>
33 <ifStmt>     → if <expr> then <body>
34 <ifStmt>     → if <expr> then <body> else <elsePart>   *
35 <elsePart>   → <ifStmt>
36 <elsePart>   → <body>
37 <ifExpr>     → if <expr> then <body> else <elsePart>
38 <while>      → while <expr> do NEWLINE INDENT <block> DEDENT
39 <for>        → for <forHeader> do <body>
40 <forHeader>  → id <- <expr> (to|until) <expr> [by <expr>] <guards>
41 <forHeader>  → NEWLINE INDENT id <- <expr> (to|until) <expr>
                  [by <expr>] <mlGuards> DEDENT
42 <guards>     → if <expr> <guards>
43 <guards>     → ε
44 <mlGuards>   → NEWLINE if <expr> <mlGuards>   *
45 <mlGuards>   → NEWLINE
46 <body>       → NEWLINE INDENT <block> DEDENT
47 <body>       → <statement>
48 <exprOrCall> → <ifExpr>
49 <exprOrCall> → id ( <argList> )               *
50 <exprOrCall> → <expr>
51 <argList>    → <arg> <argRest>
52 <argList>    → ε
53 <argRest>    → , <arg> <argRest>
54 <argRest>    → ε
55 <arg>        → <term> (. id)*
56 <term>       → id | int | double | string | bool
```

### LL table

For readability the table is split into three parts by non-terminal
groups. `e` = expr-start tokens (literal, `(`, `!`, unary `-`),
`T` = type keywords (Int, Double, String, Unit, Boolean),
`kw` = `var`/`val`. An empty cell means a syntax error.

Top-level rules:

| | import | @ | def | id | ) | , | : | T | if | NL | EOF | e |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| **program** | 1 | | | | | | | | | | | |
| **prolog** | 2 | | | | | | | | | | | |
| **funcList** | | 3 | 3 | | | | | | | | 4 | |
| **funcDef** | | 5 | 5 | | | | | | | | | |
| **funcBody** | | | | 7 | | | | | 7 | 6 | | 7 |
| **paramList** | | | | 8 | 9 | | | | | | | |
| **paramRest** | | | | | 11 | 10 | | | | | | |
| **param** | | | | 12 | | | | | | | | |
| **type** | | | | | | | | 13 | | | | |

Statements and bodies:

| | id | kw | if | else | while | for | return | NL | ; | DEDENT | e |
|---|---|---|---|---|---|---|---|---|---|---|---|
| **block** | 14 | 14 | 14 | | 14 | 14 | 14 | | | | 14 |
| **stmtSeq** | 15/16 | 15/16 | 15/16 | | 15/16 | 15/16 | 15/16 | | | | 15/16 |
| **sep** | | | | | | | | 17 | 18 | | |
| **statement** | 24 | 19 | 20 | | 21 | 22 | 23 | | | | 25 |
| **varDecl** | | 26 | | | | | | | | | |
| **idStmt** | 27/28/29* | | | | | | | | | | |
| **return** | | | | | | | 30/31* | | | | |
| **exprStmt** | | | | | | | | | | | 32 |
| **ifStmt** | | | 33/34* | | | | | | | | |
| **elsePart** | 36 | 36 | 35 | | 36 | 36 | 36 | 36 | | | 36 |
| **ifExpr** | | | 37 | | | | | | | | |
| **while** | | | | | 38 | | | | | | |
| **for** | | | | | | 39 | | | | | |
| **body** | 47 | 47 | 47 | | 47 | 47 | 47 | 46 | | | 47 |

The `for` header and function calls:

| | id | ( | ) | , | if | do | NL | e |
|---|---|---|---|---|---|---|---|---|
| **forHeader** | 40 | | | | | | 41 | |
| **guards** | | | | | 42 | 43 | | |
| **mlGuards** | | | | | | | 44/45* | |
| **exprOrCall** | 49/50* | 50 | | | 48 | | | 50 |
| **argList** | 51 | | 52 | | | | | 51 |
| **argRest** | | | 54 | 53 | | | | |
| **arg** | 55 | | | | | | | 55 |
| **term** | 56 | | | | | | | 56 |


# Modules description

## Code generator (codegen)

The code generator builds on the parser module with semantic analysis, from which it receives the completed AST. The output is a sequence of instructions defined by IFJcode26 to standard output. Main files are *codegen.c* and *codegen.h* with tests.

The basis of generating instructions is passing through the AST and writing out the corresponding instructions from AST nodes to standard output directly.

### Variables and functions

- Names of all compiler-only variables and functions use '$' in name for separation from user-code names.
- To ensure unique labels for separate instructions (like two for loops after each other), we generate unique number from global counter every time needed.
- Shorter built-in functions of IFJcode26 are generated in-line, but the longer ones are designed as a normal function you call, as described in _codegen.c_ header.
- When calling a function, arguments are pushed to the stack, so that the last argument is at the top. Return value is left at stack top.

### Complications and solutions

The source code IFJ26 supports variable shadowing but IFJcode26 does not. Therefore we used unique ID for every variable in source code to distinguish them.

We perform "pre-pass" of the AST functions, where we generate all the "DEFVAR" instructions, so that they are accessible throughout all sub-blocks.

Interpretation of every AST node is different with the type of the node:
- function definitions (including @main; we use the "pre-pass" here)
- blocks (for example function body)
- statements (for example function calls, while loop or '=')
- expressions (everything else, leaves value on stack)

### Notes

- The module assumes semantic correctness and does not check it.
- The module does not perform general optimalization (except for the specified ones in IFJcode26 specificaton, like && and || generation).
- Code generator itself does not use symbol table. All data needed like data types, conversions or scopes are embedded in AST nodes.


## Symbol table and Hash table

Symbol table manages variables and functions in scopes, where each scope has a reference to its parent (linked list), so when searching for a symbol, the newest is prefered (ensures shadowing). Each scope has its own hash table. Once the nested scope is removed, shadowed variable from parent scope is avalible again.

Due to this behaviour, symbol table is basically empty at the end of semantic analysis, and therefore code generator can not use it to look up any symbols.

Functions are stored in global scope. Every variable (including parameters) also contains 'UID' which is generated by semanntic analysis and also put into AST. Then code generator uses it to distinguish between different shadowed variables from AST only. 

### Hashing

The hash table uses the DJB2 hashing function ([Daniel J. Bernstein: djb2 hash](https://theartincode.stanis.me/008-djb2/); produces different hash for the same strings in a different character order). Hash table uses a looping array of cells (key value pairs with status) as memory.

Inserting element from hash table works by:
- hashing the key to get index of element
- passing through array, skipping deleted cells
- if a cell with same key is found, it is an unallowed redefinition
- if a empty cell is found, element is inserted (unless deleted cell was found first - element is inserted at first deleted cell spot)

Freeing an element is ensured by the caller by defining a callback free function the hashtable then uses.


## Semantic analysis

The module utilizes the AST created by the parser and the symbol table, where all the user variables and functions are saved. We use 2 pass architecture with pre-definition of builtin functions:

0. Insertion of built-in functions with parameters into symbol table.
1. Analysis of user defined functions with parameters and types (and insertion into symbol table).
2. Analysis of function bodies.

We also generate unique id for all variables including function parameters. This ensures that the code generator does not need symbol table, which would be practically empty at the end of semantic analysis due to scope popping.

# References

Daniel J. Bernstein. *djb2 hash*. <https://theartincode.stanis.me/008-djb2/>

# AI usage
- We agreed at a team meeting to document every use of AI and to briefly describe what it was used for, as required by the assignment.
