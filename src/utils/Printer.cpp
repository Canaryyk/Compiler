#include "Printer.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <vector>
#include "../parser/Parser.h"

void Printer::print_lexical_output(const std::vector<Token>& tokens, SymbolTable& symbol_table) {
    std::cout << "词法分析阶段输出:" << std::endl;
    std::cout << "--------------------" << std::endl;

    // 1. 打印Token序列
    std::cout << "Token序列:" << std::endl;
    for (const auto& token : tokens) {
        char category_char = ' ';
        switch (token.category) {
            case TokenCategory::KEYWORD:    category_char = 'k'; break;
            case TokenCategory::IDENTIFIER: category_char = 'i'; break;
            case TokenCategory::CONSTANT:   category_char = 'c'; break;
            case TokenCategory::OPERATOR:   category_char = 'p'; break;
            case TokenCategory::END_OF_FILE: continue; // 不打印EOF
            default:                        category_char = '?'; break;
        }
        if (token.category != TokenCategory::END_OF_FILE) {
             std::cout << "(" << category_char << ", " << token.index << "), ";
        }
    }
    std::cout << std::endl << std::endl;

    // 2. 打印关键字K表和界符P表
    std::cout << "关键字K表 和 界符P表:" << std::endl;
    std::cout << std::left << std::setw(10) << "编号" << std::setw(15) << "关键字" 
              << std::setw(10) << "编号" << std::setw(15) << "界符" << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
    
    const auto& keywords = symbol_table.get_keyword_table();
    const auto& operators = symbol_table.get_operator_table();
    size_t max_rows = std::max(keywords.size(), operators.size());

    for (size_t i = 0; i < max_rows; ++i) {
        if (i < keywords.size()) {
            std::cout << std::left << std::setw(10) << (i + 1) << std::setw(15) << keywords[i];
        } else {
            std::cout << std::left << std::setw(25) << " ";
        }
        if (i < operators.size()) {
            std::cout << std::left << std::setw(10) << (i + 1) << std::setw(15) << operators[i];
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // 3. 打印标识符I表
    std::cout << "标识符I表:" << std::endl;
    std::cout << std::left << std::setw(10) << "编号" << std::setw(15) << "NAME" << std::endl;
    std::cout << "--------------------" << std::endl;
    const auto& identifiers = symbol_table.get_simple_identifier_table();
    for (size_t i = 0; i < identifiers.size(); ++i) {
        std::cout << std::left << std::setw(10) << (i + 1) << std::setw(15) << identifiers[i] << std::endl;
    }
    std::cout << std::endl;

    // 4. 打印常数C表
    std::cout << "常数C表:" << std::endl;
    std::cout << std::left << std::setw(10) << "编号" << std::setw(15) << "VALUE" << std::endl;
    std::cout << "--------------------" << std::endl;
    const auto& constants = symbol_table.get_constant_table();
    for (size_t i = 0; i < constants.size(); ++i) {
        std::cout << std::left << std::setw(10) << (i + 1) << std::setw(15) << constants[i] << std::endl;
    }
    std::cout << std::endl;
}

// Helper to convert Operand to string
std::string operand_to_string(const Operand& op, SymbolTable& symbol_table) {
    if (op.type == Operand::Type::NONE) {
        return "-";
    }
    
    std::stringstream ss;
    switch(op.type) {
        case Operand::Type::IDENTIFIER:
            ss << op.name;
            break;
        case Operand::Type::TEMPORARY:
            ss << op.name;
            break;
        case Operand::Type::CONSTANT:
            ss << symbol_table.get_constant_table()[op.index];
            break;
        case Operand::Type::LABEL:
            ss << "L" << op.index;
            break;
        case Operand::Type::NONE:
            ss << "-";
            break;
    }
    return ss.str();
}

void Printer::print_semantic_output(const std::vector<Quadruple>& quadruples, SymbolTable& symbol_table) {
    std::cout << "中间代码生成阶段输出:" << std::endl;
    std::cout << "------------------------" << std::endl << std::endl;

    // 1. 打印四元式区
    std::cout << "四元式区:" << std::endl;
    std::cout << "--------------------" << std::endl;
    for (size_t i = 0; i < quadruples.size(); ++i) {
        const auto& q = quadruples[i];
        std::cout << std::left << std::setw(4) << i
                  << "(" << std::setw(4) << opcode_to_string(q.op) << ", "
                  << std::setw(8) << operand_to_string(q.arg1, symbol_table) << ", "
                  << std::setw(8) << operand_to_string(q.arg2, symbol_table) << ", "
                  << std::setw(8) << operand_to_string(q.result, symbol_table) << ")" << std::endl;
    }
    std::cout << std::endl;

    // 2. 打印符号表
    std::cout << "符号表 I 表:" << std::endl;
    std::cout << std::left << std::setw(5) << " "
              << std::setw(15) << "NAME"
              << std::setw(10) << "TYPE"
              << std::setw(10) << "CAT"
              << std::setw(10) << "ADDR" << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
    const auto& entries = symbol_table.get_symbol_entries();
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        std::string type_str = "unknown";
        if (entry.type->size == 4) type_str = "i"; // integer
        else if (entry.type->size == 8) type_str = "r"; // real

        std::string cat_str = "v"; // variable

        std::cout << std::left << std::setw(5) << (i + 1)
                  << std::setw(15) << entry.name
                  << std::setw(10) << type_str
                  << std::setw(10) << cat_str
                  << std::setw(10) << entry.address << std::endl;
    }
    std::cout << std::endl;

    // 3. 打印常数表
    std::cout << "常数C表:" << std::endl;
    std::cout << std::left << std::setw(10) << "编号" << std::setw(15) << "VALUE" << std::endl;
    std::cout << "--------------------" << std::endl;
    const auto& constants = symbol_table.get_constant_table();
    for (size_t i = 0; i < constants.size(); ++i) {
        std::cout << std::left << std::setw(10) << (i + 1) << std::setw(15) << constants[i] << std::endl;
    }
    std::cout << std::endl;
    
    // 4. 打印活动记录快照
    std::cout << "活动记录映像:" << std::endl;
    std::cout << "--------------------" << std::endl;
    // 使用 map 按地址排序
    std::map<int, std::string> memory_layout;
    for(const auto& entry : entries) {
        memory_layout[entry.address] = entry.name;
    }

    for (auto const& [addr, name] : memory_layout) {
        std::cout << std::left << std::setw(5) << addr << "| " << name << std::endl;
    }
    std::cout << "--------------------" << std::endl;

} 