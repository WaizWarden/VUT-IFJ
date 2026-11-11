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

Parser keeps the actual token and also the token ahead of it (needed for the references) - process called "Look-ahead"

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

The block can be defined as a expression (`x = 5 + 3`) or by a function call (`x = f(y)`). The definition can be on the same line or be indnted.

Anotation of `"@main"` is only noted by parser into the AST. The corectness of sematics is not checked by parser.

### Block 

For blocks, there are 2 specific commands: `"INDEND"` and `"DEDENT"`.

  - `"INDEND"` is a unit of functing nesting further into the block
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
