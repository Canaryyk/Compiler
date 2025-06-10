#ifndef QUADRUPLE_H
#define QUADRUPLE_H

#include <string>
#include <variant>
#include "../semantic_analyzer/SymbolTable.h"

enum class OpCode {
    // 算术运算
    ADD, SUB, MUL, DIV,
    // 赋值
    ASSIGN,
    // 关系运算
    EQ, NE, LT, LE, GT, GE,
    // 跳转
    JUMP, JUMP_IF_FALSE,
    // 过程调用
    PARAM, CALL, RET,
    // IO 操作 (示例)
    PRINT,
    // 内存操作
    NO_OP // 无操作，用于优化过程中的占位符
};


// 操作数
struct Operand {
    enum class Type {
        IDENTIFIER,
        CONSTANT,
        TEMPORARY,
        LABEL
    };

    Type type;
    int index; // 在对应表中的索引

    // 为了方便调试，可以添加一个名称
    std::string name; 
};


// 四元式
struct Quadruple {
    OpCode op;
    Operand arg1;
    Operand arg2;
    Operand result;
};


#endif // QUADRUPLE_H 