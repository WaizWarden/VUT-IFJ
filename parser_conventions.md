# Conversion of tokens given by Scanner to AST through Parser

## Basic grammar:
````
Program -> prolog, FunDefinition[]
FunDefinition -> name, params[], retType | INFER, isMain?, body
    body -> expression | Call | Block
If -> cond, thenBlock, elseBlock
While -> cond, loopBlock
Return -> expression | Call | ∅
Expression -> Binary, Unary, Literal, Identifier
Binary -> operation, leftSide, rightSide
Unary -> operation, operand
Literal -> Int, Double, String, Unit
Block -> Stmt[]
Stmt -> VarDef, Assign, Call, Return, If, While, ExprStmt
VarDef -> name, isMutable(var/val)?, initExpression, declaredType | INFER
Call -> name, args[]
Assign -> name, expression
Parameters -> (name, type)[] 

````

## Example of AST generated on a sample code
```c++
import ifj26.*

def factorial(n: Int): Int =
    var result = 1
    while n > 1 do
        result = result * n
        n = n - 1
    result

@main def run(): Unit =
    val n = readInt()
    val r = factorial(n)
    print(r)
```

## Converted AST based on the sample code

```
Program
   |
   |- FunDefinition "factorial" retType=Int isMain=false
   |  |- Parameters
   |  |   |- Param: name="n" type=Int
   |  |
   |  |- Block
   |      |- VarDef name="result"  mutable=var  declType=INFER
   |      |   |- init: Literal INT 1
   |      |
   |      |- While
   |      |   |- cond: Binary op=GT
   |      |   |   |- left:  IDENT "n"
   |      |   |   |- right: Literal INT 1
   |      |   |
   |      |   |- loopBlock: Block
   |      |       |- Assign name="result"
   |      |       |   |- expr: Binary op=MUL
   |      |       |       |- left:  IDENT "result"
   |      |       |       |- right: IDENT "n"
   |      |       |
   |      |       |- Assign name="n"
   |      |           |- expr: Binary op=SUB
   |      |               |- left:  IDENT "n"
   |      |               |- right: Literal INT 1
   |      |
   |      |- ExprStmt
   |          |- IDENT "result"
   |
   |- FunDefinition "run" retType=Unit isMain=true
       |- Parameters (empty)
       |
       |- Block
           |- VarDef name="n"  mutable=val  declType=INFER
           |   |- init: CALL  "readInt"   args=[]
           |
           |- VarDef name="r"  mutable=val  declType=INFER
           |   |- init: CALL  "factorial"   args=[ IDENT "n" ]
           |
           |- Call "print"
               |- args: [ IDENT "r" ]
```

## Operators and precedence (from spec, highest first)
```
Prio | Operator          | AST node                 | Associativity          | Parser rule
1    | `-x` (unary)      | `Unary OP_UNARY_MINUS`   | right (operand za ním) | `parse_unary` 
2    | `*` `/`           | `Binary OP_MUL` `OP_DIV` | left                   | `parse_multiplicative` 
3    | `+` `-`           | `Binary OP_ADD` `OP_SUB` | left                   | `parse_additive` 
4    | `<` `<=` `>` `>=` | `Binary OP_LT/LE/GT/GE`  | **none**               | `parse_comparison` (if, not while) 
5    | `==` `!=`         | `Binary OP_EQ` `OP_NE`   | **none**               | `parse_equality` (if, not while) 
```

# Rules given by specs
### 1. Where can Call/Expression appear?
```
- Call is NOT part of Expression (no FUNEXP). It may appear only as:
  Stmt | VarDef.init | Assign.expr | Return value | single-line funDef body
- Call args = terms only (Literal | Identifier), not full expressions
- Expression may appear: VarDef.init, Assign.expr, Return, If.cond,
      While.cond, ExprStmt, funDef body, inside parens
```

### 2. Parser interface
```
- Consumes tokens via lookahead: current + lookahead over scanner_next()
- TOKEN_ERROR from scanner -> exit(1) (lexical)
- Grammar mismatch -> exit(2) (syntax)
- Ownership: token.value.identifier/string moves INTO the AST node;
  ast_free() frees the whole tree recursively
- Every node stores `line` (for semantic errors 3-7 later)
```

### 3. Structural rules worth noting:
```
- else branch is MANDATORY (base variant); else must be on its own line
  with same indent as if
- Blocks are delimited by INDENT/DEDENT tokens; empty block -> syntax error
  (use `()` literal as body)
- Stmts separated by NEWLINE or `;`
- One-line funDef: `def f(x: Int) = expr` -> body is expression, not Block
- Program must contain exactly one isMain=true funDef (parser just marks
  the flag; "exactly one" check can stay in semantics)
```

### 4. Example of precedence
```
m-4*5
Binary SUB
   |- left:  IDENT "n"
   |- right: Binary MUL (Literal 4, Literal 5)
```