/**
 * @file    ast.h
 * @author  xhorakj01
 * @brief   AST header file.
 */

#ifndef AST_H
#define AST_H

typedef enum {
    AST_BINARY,
    AST_UNARY,
    AST_LITERAL_INT,
    AST_LITERAL_DOUBLE,
    AST_IDENTIFIER,
    AST_ASSIGN
}AstNodeType;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
}BinaryOperation;

typedef enum{
    OP_UNARY_MINUS
}UnaryOperation;

typedef struct AstNode{
    AstNodeType type;
    int line;
    union{
        struct{
            BinaryOperation operation;
            struct AstNode *left;
            struct AstNode *right;
        }binary;

        struct{
            UnaryOperation op;
            struct AstNode *operand;
        }unary;

        struct{
            char *name;
            struct AstNode *value;
        }assign;

        int intValue;
        double doubleValue;
        char *identif;
    }as;
}AstNode;

/**
* @brief Creates a binary operation node (takes ownership of children)
*/
AstNode *ast_new_binary(BinaryOperation op, AstNode *left, AstNode *right, int line);

AstNode *ast_new_unary(UnaryOperation op, AstNode *operand, int line);
AstNode *ast_new_int(int value, int line);
AstNode *ast_new_double(double value, int line);
AstNode *ast_new_identifier(char *name, int line);  // steals `name`
AstNode *ast_new_assign(char *name, AstNode *value, int line);

void ast_free(AstNode *node);
void ast_print(const AstNode *node, int indent);

#endif //AST_H
