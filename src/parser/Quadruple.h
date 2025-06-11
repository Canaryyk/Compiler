/**
 * @file Quadruple.h
 * @brief 定义了用于中间代码生成的四元式结构。
 *
 * 四元式是一种常见的中间表示形式，格式为 (operator, argument1, argument2, result)。
 * 它将源程序的复杂结构分解为一系列简单的、类似汇编语言的指令。
 */
#ifndef QUADRUPLE_H
#define QUADRUPLE_H

#include <string>
#include <variant>
#include "../semantic_analyzer/SymbolTable.h"

/**
 * @brief 四元式中的操作码，定义了指令的类型。
 */
enum class OpCode {
    // --- 算术运算 ---
    ADD, ///< 加法 (result = arg1 + arg2)
    SUB, ///< 减法 (result = arg1 - arg2)
    MUL, ///< 乘法 (result = arg1 * arg2)
    DIV, ///< 除法 (result = arg1 / arg2)
    
    // --- 赋值 ---
    ASSIGN, ///< 赋值 (result = arg1)
    
    // --- 关系运算 ---
    EQ, ///< 等于 (result = (arg1 == arg2))
    NE, ///< 不等于 (result = (arg1 != arg2))
    LT, ///< 小于 (result = (arg1 < arg2))
    LE, ///< 小于等于 (result = (arg1 <= arg2))
    GT, ///< 大于 (result = (arg1 > arg2))
    GE, ///< 大于等于 (result = (arg1 >= arg2))
    
    // --- 跳转指令 ---
    JMP,            ///< 无条件跳转 (goto result)
    JPF,   ///< 条件为假则跳转 (if not arg1 then goto result)
    
    // --- 过程/函数调用 ---
    PARAM,  ///< 为函数/过程调用传递参数 (pass arg1)
    CALL,   ///< 调用一个过程或函数 (call arg1, arg2=num_params, result=return_val_temp)
    RETURN, ///< 从一个函数返回一个值 (return arg1)
    
    // --- 占位符 ---
    NO_OP  ///< 空操作，不执行任何动作
};


/**
 * @brief 代表四元式中的一个操作数。
 * 它可以是变量、常量、临时变量或一个跳转目标（标签）。
 */
struct Operand {
    /**
     * @brief 操作数的类型。
     */
    enum class Type {
        IDENTIFIER, ///< 标识符（变量/函数名等）。
        CONSTANT,   ///< 常量。
        TEMPORARY,  ///< 临时变量，由编译器在计算表达式时生成。
        LABEL       ///< 标签，用于表示跳转指令的目标地址。
    };

    Type type; ///< 操作数的具体类型。
    
    /**
     * @brief 在对应表中的索引。
     * - 对于 IDENTIFIER: 是其在符号表 `symbol_entries` 中的地址 `address`。
     * - 对于 CONSTANT: 是其在常量表 `constant_table` 中的索引。
     * - 对于 TEMPORARY: 是临时变量的唯一编号。
     * - 对于 LABEL: 是跳转目标的四元式索引。
     */
    int index; 

    /**
     * @brief 操作数的可读名称，主要用于调试和输出。
     * 例如，可以是变量名 "x"，临时变量名 "t1"，或标签名 "L2"。
     */
    std::string name; 
};


/**
 * @brief 四元式结构体，定义了一个独立的中间代码指令。
 */
struct Quadruple {
    OpCode op;      ///< 操作码。
    Operand arg1;   ///< 第一个参数。
    Operand arg2;   ///< 第二个参数。
    Operand result; ///< 结果存放处或跳转目标。
};


#endif // QUADRUPLE_H 