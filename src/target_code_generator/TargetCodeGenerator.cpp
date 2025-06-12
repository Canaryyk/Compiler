#include "TargetCodeGenerator.h"
#include <map>
#include <iostream>

TargetCodeGenerator::TargetCodeGenerator() {}

// Helper to get the name of an operand.
// This simplifies the logic from the user-provided code.
static std::string get_operand_name(const Operand& op, const SymbolTable& table) {
    if (op.type == Operand::Type::CONSTANT) {
        if (op.index >= 0 && static_cast<size_t>(op.index) < table.get_constant_table().size()) {
            return std::to_string(static_cast<int>(table.get_constant_table()[op.index]));
        }
    }
    return op.name;
}

std::vector<TargetCodeLine> TargetCodeGenerator::generate(const std::vector<Quadruple>& quads, SymbolTable& symbol_table) {
    std::vector<TargetCodeLine> target_code;
    std::string reg = "R0"; // 使用单个寄存器
    std::map<int, int> quad_to_code_line; // <四元式行号, 目标代码行号>
    std::vector<std::pair<int, int>> backpatch_list; // <需要回填的目标代码行, 跳转到的四元式行号>

    // 第一次遍历：生成指令并记录跳转信息
    for (size_t i = 0; i < quads.size(); ++i) {
        const auto& quad = quads[i];
        quad_to_code_line[i] = target_code.size();

        std::string arg1_name = get_operand_name(quad.arg1, symbol_table);
        std::string result_name = get_operand_name(quad.result, symbol_table);

        switch(quad.op) {
            case OpCode::ASSIGN:
                target_code.push_back({-1, "LD " + reg + ", " + arg1_name});
                target_code.push_back({-1, "ST " + reg + ", " + result_name});
                break;
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::LT:
            case OpCode::GT:
            case OpCode::LE:
            case OpCode::GE:
            case OpCode::EQ:
            case OpCode::NE:
            {
                std::string arg2_name = get_operand_name(quad.arg2, symbol_table);
                target_code.push_back({-1, "LD " + reg + ", " + arg1_name});
                target_code.push_back({-1, opcode_to_string(quad.op) + " " + reg + ", " + arg2_name});
                target_code.push_back({-1, "ST " + reg + ", " + result_name});
                break;
            }
            case OpCode::JMP:
                target_code.push_back({-1, "JMP "});
                backpatch_list.push_back({(int)target_code.size() - 1, quad.result.index});
                break;
            case OpCode::JPF: // j< in user code
                target_code.push_back({-1, "FJ " + arg1_name});
                backpatch_list.push_back({(int)target_code.size() - 1, quad.result.index});
                break;
            case OpCode::PARAM:
                target_code.push_back({-1, "LD " + reg + ", " + arg1_name});
                target_code.push_back({-1, "PARAM"});
                break;
            case OpCode::CALL:
                target_code.push_back({-1, "CALL " + quad.arg1.name + ", " + quad.arg2.name});
                if (quad.result.type != Operand::Type::NONE) {
                    target_code.push_back({-1, "ST " + reg + ", " + result_name});
                }
                break;
            case OpCode::RETURN:
                if (quad.arg1.type != Operand::Type::NONE) {
                     target_code.push_back({-1, "LD " + reg + ", " + arg1_name});
                }
                target_code.push_back({-1, "RET"});
                break;
            case OpCode::LABEL:
            case OpCode::PRINT:
            case OpCode::NO_OP:
            case OpCode::NONE:
                // 在这个简单的模型中忽略这些
                break;
        }
    }

    // 第二次遍历：回填跳转地址
    for (const auto& patch : backpatch_list) {
        int code_index = patch.first;
        int target_quad_index = patch.second;

        if (quad_to_code_line.count(target_quad_index)) {
            int target_line = quad_to_code_line[target_quad_index];
            target_code[code_index].code += " L" + std::to_string(target_line);
        } else {
            target_code[code_index].code += " ?"; // 未知目标
        }
    }
    
    // 最后，为所有行分配行号
    for(size_t i = 0; i < target_code.size(); ++i) {
        target_code[i].line_number = i;
    }

    return target_code;
} 