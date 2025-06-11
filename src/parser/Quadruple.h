#ifndef QUADRUPLE_H
#define QUADRUPLE_H

#include <string>
#include <variant>
#include "../semantic_analyzer/SymbolTable.h"

// 操作码
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
    NO_OP,  ///< 空操作，不执行任何动作
    PRINT,
    NONE,
    LABEL
};

// 将 OpCode 转换为字符串表示，方便打印
std::string opcode_to_string(OpCode op);


// 操作数
struct Operand {
    enum class Type {
        IDENTIFIER,
        CONSTANT,
        TEMPORARY,
        LABEL,
        NONE
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