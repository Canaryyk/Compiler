#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <algorithm>
#include "semantic_analyzer/SymbolTable.h"
#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "utils/Printer.h"
#include "optimizer/Optimizer.h"

int main() {
    std::string file_path = "examples/test_lexical.txt";
    std::ifstream file_stream(file_path);
    if (!file_stream) {
        std::cerr << "Error: Cannot open file " << file_path << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    std::string source_code = buffer.str();

    // --- 词法分析 ---
    SymbolTable symbol_table;
    Lexer lexer(source_code, symbol_table);
    // 完整地运行一次词法分析，以填充用于打印的简单标识符表
    Lexer lexer_for_print(source_code, symbol_table);
    Token token_for_print;
    do {
        token_for_print = lexer_for_print.get_next_token();
    } while (token_for_print.category != TokenCategory::END_OF_FILE);
    
    // 打印词法分析结果
    Printer::print_lexical_output(lexer_for_print.get_all_tokens(), symbol_table);
    std::cout << "\n==============================================\n\n";

    // --- 语法、语义分析和中间代码生成 ---
    try {
    Parser parser(lexer, symbol_table);
    parser.parse();
    auto raw_quads = parser.get_quadruples();
    
    // 在 parser.parse() 之后
    std::cout << "Raw quads size: " << raw_quads.size() << std::endl;
    
    // 打印原始四元式
    std::cout << "\n原始四元式：" << std::endl;
    Printer::print_semantic_output(raw_quads, symbol_table);
    
    try {
        // 调用优化器
        auto optimized_quads = Optimizer::optimize(raw_quads, symbol_table);

        // 在优化之后
        std::cout << "\n优化后四元式：" << std::endl;
        std::cout << "Optimized quads size: " << optimized_quads.size() << std::endl;

        // 打印优化后中间代码
        Printer::print_semantic_output(optimized_quads, symbol_table);
    } catch (const std::exception& e) {
        std::cerr << "优化过程中出现错误: " << e.what() << std::endl;
        std::cerr << "将使用未优化的四元式继续执行。" << std::endl;
        // 使用原始四元式继续
        Printer::print_semantic_output(raw_quads, symbol_table);
    }
  } 
    catch (const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
    return 1;
    }
    return 0;
}
