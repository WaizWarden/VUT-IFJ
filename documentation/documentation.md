# Project overview

**Implemented extensions:** BOOLTHEN, CONTROL, VALMEM, INTERPOLATEDS.

## Team distribution
- **xjuricl01** – project coordination, lexical analysis, fixes in semantic analysis
- **xoramst00** – symbol table, code generator (main developer)
- **xhorakj01** – parser, AST, precedence analysis, stack
- **xsirmat00** – semantic analysis, code generator (secondary developer), dropped out

### Explanation of percent distribution
**xsirmat00** dropped out and will retake the course; per his agreement with the team he receives 0 %.

## Team collaboration
- Each module has its own unit tests
- Whole compiler is tested by e2e tests
- Before merging to main we used valgrind to check for memory leaks
- We used `clang-tidy` for extra warnings detection
- For formating we used `clang-format`
- We didn't use PR requests


## Block diagram

![Compiler architecture and module interactions](documentation/img/Block-diagram.png)

# Lexical Analysis
Lexical analysis is the process of reading human-readable code and convets it to a sequence that a parser can work with.
In the process, it removes unnecessary parts of the code, such as comments and whitespace.


## Architecture
- `scanner.c` - main process, goes through input and converts it into tokens
- `scanner_stack.c` - handles indent and dedents of the code

## FSM of scanner

<img src="./documentation/fsmDedents.png"
     style="width: 100%; height: auto;" />
  
<div class="landscape" style="text-align: center;">
  <img src="./documentation/fsm.png"
       style="height: 19.8cm; width: auto; max-width: 100%;" />
</div>

### notes
- Transitions not shown in the diagram lead to an error

- END = {+ - * / = < > ! ( ) ; , whitespace | & } EOF}
- T_DOUBLE and T_INT are complete when the next character belongs to END group, the character is return to the input

- To determine if token is T_KEYWORD or T_IDENTIFER, we check it against array of keywords

- If scanner gets to end of file (T_EOF in fsm), he will first return all pendingDedents
- Every time NEW_LINE token is returned, lineStart is set to true (for indent calculation)

- Interpolation extension

- At start, flags are checked first 
	- interpStart -> T_INTERP_START
	- interpEnd -> T_INTERP_END
	- interpResume -> scan rest of the string

- When the scanner finds a letter or _ after $, it sets interpVariable = true and interpStart = true
- When the scanner finds { after $, it sets interpStart = true and interpExprDepth++.
- When an identifier ends and interpVariable is true, it sets interpVariable = false, interpEnd = true and interpResume = true.

- Multi-line interpolated strings works the same way (they set the same flags)


# Syntax Analysis

## Architecture

Scanner produces tokens, which are then processed by syntax analysis, output is the AST tree.

Structure of programm is processed by the `parser.c` using the recursive descent. Expressions are managed in `precedence.c` using precedence analysis.

The product of syntax analysis is `AST_PROGRAM` and syntax errro ends with error 2.

## Recursive descent algorithm

Each parser function handles one syntactic construct (function definitions, variable declarations, the prolog, loops, conditions). Based on the current token it decides which rule to apply and what to expect next. 

The product are nodes of AST tree, if the syntax is incorrect, programm ends with exit code 2.

### Prolog, Program and definition of functions

Parser skips any leading newlines and expects the prolog.
Prolog is necceceary string contained of `import ifj26.*`.

Prolog is mandatory exactly once in the beginning of programm.

The parser then processes function definitions one by one until it reaches EOF. The resulting nodes are stored in the root AST node (representing programm).

#### How is the function body defined

The body can be a single expression (`def f(): Int = 5 + 3`), a function call (`def g(): Int = f()`), or an indented block. The definition can be on the same line or on indented lines below. (using NEWLINE INDENT, DEDENT)

The `@main` annotation is only recorded in the AST by the parser; its semantic correctness is not checked here.

### Block 

Scanner generates tokens based on the indentation.

  - `INDENT` marks nesting one level deeper into a block
  - `DEDENT` marks leaving the block

For the structure of blocks parser does not use spaces and tabs, but only INDENT and DEDENT.

#### Inside of block

Inside a block, the parser processes statements.
Simple statements are separated by a newline or a semicolon.
A block must not be empty.

#### Distinguishing expressions and function calls

Standalone expressions are not handled by `parser.c` itself but are passed to `precedence.c`.
Function calls are parsed by `parser.c` directly.

This is the result of absence of extension FUNEXP.

If a statement starts with an identifier, the parser stores its name and looks at the next token to decide:

- `=` - an assignment to the variable
- `(` - a function call
- anything else - an expression starting with that identifier

When handing the expression over to precedence analysis, the parser also passes the AST node for the already-read identifier, so the precedence parser can continue without re-reading it.

Call arguments are parsed as a comma-separated list of terms, not as general expressions.

Inside call arguments, member access `arg.member` is supported (VALMEM).

## Operator-Precedence Parsing

Module `precedence.c` is independant (as module on its own) from `parser.c`
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

![Precedence relation actions](documentation/img/precedence-actions.png)

On reduction the analysis pops elements up to the `<` marker. If they match a reduction rule, they are replaced by one nonterminal carrying the new AST node. No new input token is loaded during a reduction.

Reduction links AST subtrees into new nodes.

### Precedence table

![Precedence table](documentation/img/precedence-table.png)

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
      - No additional AST node is created (else block is represented as nested AST_IF)

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

Expressions are outside the LL grammar and `<expr>` is a complete expression parsed by precedence analysis. Two interface symbols are used at the identifier boundary: `<exprNoId>` is an expression, its first token is not an identifier, and `<exprTail>` is the continuation of an expression after its first identifier has already been consumed by the LL parser. `type` denotes any type keyword, `atom` denotes an identifier or a literal.

Precedence grammar for `<expr>` (not part of the LL grammar):

```
E → i | E +- E | E */ E | u-E | !E | E rel E | E ==!= E
  | E && E | E || E | E . id | ( E ) | ( )
```

### Numbered rules

```
 1 <program>       → <prolog> NL <funcList> EOF
 2 <prolog>        → import ifj26 . *
 3 <funcList>      → <funcDef> <funcList>      4 <funcList>      → ε
 5 <funcDef>       → <mainOpt> def id ( <paramList> ) <typeOpt> = <funcBody>
 6 <mainOpt>       → @ main                    7 <mainOpt>       → ε
 8 <typeOpt>       → : type                    9 <typeOpt>       → ε
10 <funcBody>      → NL IND <block> DED       11 <funcBody>      → <exprOrCall>
12 <paramList>     → <param> <paramRest>      13 <paramList>     → ε
14 <paramRest>     → , <param> <paramRest>    15 <paramRest>     → ε
16 <param>         → id : type
17 <block>         → <statement> <stmtTail>
18 <stmtTail>      → NL <moreStmts>           19 <stmtTail>      → ; <moreStmts>
20 <stmtTail>      → ε                        22 <moreStmts>     → ε
21 <moreStmts>     → <statement> <stmtTail>
23 <statement>     → <varDecl>                24 <statement>     → <ifStmt>
25 <statement>     → <while>                  26 <statement>     → <for>
27 <statement>     → <return>                 28 <statement>     → <idStmt>
29 <statement>     → <exprNoId>
30 <varDecl>       → var id <typeOpt> = <exprOrCall>
31 <varDecl>       → val id <typeOpt> = <exprOrCall>
32 <idStmt>        → id <idTail>
33 <idTail>        → = <exprOrCall>           34 <idTail>        → ( <argList> )
35 <idTail>        → <exprTail>
36 <return>        → return <returnTail>
37 <returnTail>    → <exprOrCall>             38 <returnTail>    → ε
39 <ifStmt>        → if <expr> then <body> <elseOpt>
40 <elseOpt>       → else <body>              41 <elseOpt>       → ε
42 <ifExpr>        → if <expr> then <body> else <body>
43 <while>         → while <expr> do NL IND <block> DED
44 <for>           → for <forHeader> do <body>
45 <forHeader>     → id <- <expr> <range> <expr> <byOpt> <guards>
46 <forHeader>     → NL IND id <- <expr> <range> <expr> <byOpt> NL <mlGuards> DED
47 <range>         → to                       48 <range>         → until
49 <byOpt>         → by <expr>                50 <byOpt>         → ε
51 <guards>        → if <expr> <guards>       52 <guards>        → ε
53 <mlGuards>      → if <expr> NL <mlGuards>  54 <mlGuards>      → ε
55 <body>          → NL IND <block> DED       56 <body>          → <statement>
57 <exprOrCall>    → <ifExpr>                 58 <exprOrCall>    → id <exprCallTail>
59 <exprOrCall>    → <exprNoId>
60 <exprCallTail>  → ( <argList> )            61 <exprCallTail>  → <exprTail>
62 <argList>       → <arg> <argRest>          63 <argList>       → ε
64 <argRest>       → , <arg> <argRest>        65 <argRest>       → ε
66 <arg>           → <term> <memberTail>
67 <memberTail>    → . id <memberTail>        68 <memberTail>    → ε
69 <term>          → atom                     70 <term>          → ( )
```

For `id = id` versus `id = id(...)`, the parser reads the identifier and inspects the next token. `(` selects a call, otherwise the identifier is passed to `precedence_parse` as `firstOperand`. Since FUNEXP is not implemented, this syntactic lookahead is sufficient.

### LL table

The table is organized by nonterminal groups.
  - `e` = expression start other than `id`/`if`
  - `a` = the `atom` terminal category
  - `et` = a token that can continue or end `<exprTail>`

Interface symbols `<expr>`, `<exprNoId>` and `<exprTail>` have no rows (these are parser outside of the LL parser)

Empty cell means syntax error.

Program, functions and parameters:

![LL table: program, functions and parameters](documentation/img/ll-table-program.png)

Blocks and statements:

![LL table: blocks and statements](documentation/img/ll-table-statements.png)

Control flow and loop bodies:

![LL table: control flow and loop bodies](documentation/img/ll-table-control.png)

The `for` header:

![LL table: the for header](documentation/img/ll-table-for.png)

Function calls and arguments:

![LL table: function calls and arguments](documentation/img/ll-table-calls.png)

`to`, `until` and `by` are not reserved words. In the `for` header they are recognized by the value of an identifier token, outside this context they remain normal identifiers. An `else` belongs to the nearest unmatched `if`.

# Other modules

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

The output of the semantic analysis module is fully anotated AST with evaluation types, which is passed to code generator.

# References

Daniel J. Bernstein. *djb2 hash*. <https://theartincode.stanis.me/008-djb2/>
Jakub Horák. *Devin*. <https://https://devin.ai/>

# AI usage
- We agreed at a team meeting to document uses of AI and to briefly describe what it was used for, as required by the assignment. Tests were generated by AI and reviewed by us.

*xhorakj01*: For this project, I mainly used the CLI-based AI agent [Devin](https://https://devin.ai/) and, occasionally, the Gemini LLM to validate implementation concepts. Both agents were used solely as consultants to help resolve issues that arose during the implementation of the parser and precedence rules. Neither agent contributed code directly to the repository. The code generation itself consisted solely of code snippets of logic, mostly in pseudocode.

*xoramst00*: I used AI tools such as copilot in VSCode for formatting comments, writing tests or to help me manage git repository (Dockerfile or pandoc). When I got stuck on failing tests, I used Gemini to help me find the mistake. AI was not needed for code architecture (though it helped a bit). Generally AI helped the most in static code analysis.

*xjuricl01*: I used Gemini and Claude chatbots. Main use cases were test generation and finding mistakes in code when stuck. I documented all of my AI use in-line and in headers.

*xsirmat00*: Generated all code with AI (not verified, but when we confronted him, he proposed 0 points for his participation)
