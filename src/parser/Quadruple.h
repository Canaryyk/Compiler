#ifndef QUADRUPLE_H
#define QUADRUPLE_H

#include <string>
#include <variant>
#include "../semantic_analyzer/SymbolTable.h"
#include "../../webview/json.hpp"

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

std::string opcode_to_string_for_json(OpCode op);

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

// 为 Operand::Type 提供字符串转换
inline std::string to_string(Operand::Type type) {
    switch (type) {
        case Operand::Type::IDENTIFIER: return "IDENTIFIER";
        case Operand::Type::CONSTANT: return "CONSTANT";
        case Operand::Type::TEMPORARY: return "TEMPORARY";
        case Operand::Type::LABEL: return "LABEL";
        case Operand::Type::NONE: return "NONE";
        default: return "INVALID_TYPE";
    }
}

// Helper to convert an Operand to its string representation for JSON output
inline std::string operand_to_string_for_json(const Operand& op, const SymbolTable& table) {
    if (op.type == Operand::Type::NONE) {
        return "-";
    }
    std::stringstream ss;
    switch(op.type) {
        case Operand::Type::IDENTIFIER:
        case Operand::Type::TEMPORARY:
            ss << op.name;
            break;
        case Operand::Type::CONSTANT:
            if (op.index >= 0 && static_cast<size_t>(op.index) < table.get_constant_table().size()) {
                ss << table.get_constant_table()[op.index];
            } else {
                ss << "const(" << op.index << ")";
            }
            break;
        case Operand::Type::LABEL:
            ss << op.index;
            break;
        default:
             ss << "-";
             break;
    }
    return ss.str();
}

// 为 Operand 提供 to_json (保留，以防万一)
inline void to_json(nlohmann::json& j, const Operand& o) {
    j = nlohmann::json{
        {"type", to_string(o.type)},
        {"index", o.index},
        {"name", o.name}
    };
}


// 四元式
struct Quadruple {
    OpCode op;
    Operand arg1;
    Operand arg2;
    Operand result;
};

// JSON serialization for a vector of Quads, tailored for the frontend
inline void to_json(nlohmann::json& j, const std::vector<Quadruple>& quads, const SymbolTable& table) {
    j = nlohmann::json::array();
    for(size_t i = 0; i < quads.size(); ++i) {
        const auto& q = quads[i];
        j.push_back({
            {"line", i},
            {"op", opcode_to_string(q.op)},
            {"arg1", operand_to_string_for_json(q.arg1, table)},
            {"arg2", operand_to_string_for_json(q.arg2, table)},
            {"result", operand_to_string_for_json(q.result, table)}
        });
    }
}


#endif // QUADRUPLE_H 