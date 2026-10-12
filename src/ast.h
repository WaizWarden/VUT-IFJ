/**
 * @file    ast.h
 * @author  xhorakj01
 * @brief   AST header file.
 */

#ifndef AST_H

#endif //VUT_IFJ_AST_H

typedef enum {
    AST_BINARY,
    AST_UNARY,
    AST_LITERAL_INT,
    AST_LITERAL_DOUBLE,
    AST_IDENTIFIER
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
            BinaryOperation op;
            struct AstNode *operation;
        }unary;

        int intValue;
        double doubleValue;
        char *identif;
    }as;
}AstNode;

#define AST_H
