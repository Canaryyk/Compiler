#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include "../webview/json.hpp"
#include "semantic_analyzer/SymbolTable.h"
#include "lexer/Lexer.h"
#include "lexer/Token.h"
#include "parser/Parser.h"
#include "parser/Quadruple.h"
#include "optimizer/optimizer.h"
#include "target_code_generator/TargetCodeGenerator.h"

//
// 使用方法:
// ./compiler --input <file_path> --target <tokens|quads|symbols|target_code>
//
// 示例:
// ./compiler --input ../examples/test_lexical.txt --target tokens
// ./compiler --input ../examples/test_semantic.txt --target quads
// ./compiler --input ../examples/test_semantic.txt --target symbols
// ./compiler --input ../examples/test_semantic.txt --target target_code
//

// 为新的目标代码结构添加 to_json 函数
void to_json(nlohmann::json& j, const TargetCodeLine& line) {
    j = nlohmann::json{{"line", line.line_number}, {"code", line.code}};
}

void print_usage() {
    std::cerr << "Usage: compiler --input <file_path> --target <tokens|quads|symbols|target_code>" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string input_file;
    std::string target;

    // 1. 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            input_file = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            target = argv[++i];
        }
    }

    if (input_file.empty() || target.empty()) {
        print_usage();
        return 1;
    }

    // 2. 读取输入文件
    std::ifstream file_stream(input_file);
    if (!file_stream) {
        std::cerr << "错误：无法打开文件 " << input_file << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    std::string source_code = buffer.str();

    // 3. 运行编译器并生成JSON
    try {
        SymbolTable symbol_table;
        Lexer lexer(source_code, symbol_table);
        nlohmann::json json_output;

        if (target == "tokens") {
            // 需要完整运行一次词法分析来填充所有表
            auto all_tokens = lexer.get_all_tokens();
            // 这里将调用为 vector<Token> 定制的 to_json
            to_json(json_output, all_tokens, symbol_table);

        } else if (target == "quads" || target == "symbols" || target == "target_code") {
            Parser parser(lexer, symbol_table);
            parser.parse();

            if (target == "quads") {
                const auto& original_quads = parser.get_quadruples();
                auto optimized_quads = Optimizer::optimize(original_quads, symbol_table);
                
                nlohmann::json before, after;
                to_json(before, original_quads, symbol_table);
                to_json(after, optimized_quads, symbol_table);

                json_output = {
                    {"before", before},
                    {"after", after}
                };
            } else if (target == "symbols") {
                json_output = symbol_table.to_json(); // 使用 SymbolTable 的 to_json 方法
            } else { // target == "target_code"
                const auto& original_quads = parser.get_quadruples();
                auto optimized_quads = Optimizer::optimize(original_quads, symbol_table);
                
                TargetCodeGenerator code_gen;
                auto target_code = code_gen.generate(optimized_quads, symbol_table);
                
                nlohmann::json target_code_json;
                to_json(target_code_json, target_code);
                json_output = target_code_json;
            }
        } else {
            std::cerr << "错误：无效的目标 '" << target << "'" << std::endl;
            print_usage();
            return 1;
        }

        // 4. 打印JSON到标准输出
        std::cout << json_output.dump(4) << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "发生错误： " << e.what() << std::endl;
        return 1;
    }

    return 0;
}