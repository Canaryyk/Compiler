#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "lexer/Lexer.h"
#include "parser/Parser.h"
#include "semantic_analyzer/SemanticAnalyzer.h"
#include "code_generator/CodeGenerator.h"
#include "utils/ErrorHandler.h"

int main(int argc, char* argv[]) {
    std::cout << "Compiler executable started." << std::endl;
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string source_file_path = argv[1];
    std::ifstream source_file(source_file_path);

    if (!source_file.is_open()) {
        std::cerr << "Error: Could not open file " << source_file_path << std::endl;
        return 1;
    }

    std::string line;
    std::string source_code;
    while (std::getline(source_file, line)) {
        source_code += line + "\n";
    }
    source_file.close();

    std::cout << "--- Source Code ---" << std::endl;
    std::cout << source_code << std::endl;
    std::cout << "-------------------" << std::endl;

    ErrorHandler error_handler;

    // 词法分析
    std::cout << "\n--- Lexing --- H" << std::endl;
    Lexer lexer(source_code, error_handler);
    std::vector<Token> tokens = lexer.tokenize();
    
    if (error_handler.has_errors()) {
        error_handler.print_errors();
        std::cerr << "Compilation failed due to lexical errors." << std::endl;
        return 1;
    }    
    std::cout << "Lexing completed. Tokens generated: " << tokens.size() << std::endl;
    // 输出 Tokens
    std::cout << "--- Tokens --- H" << std::endl;
    for (const auto& token : tokens) {
        std::cout << token.toString() << std::endl; // 使用 Token 类自带的 toString() 方法
    }
    std::cout << "--------------" << std::endl;

    // 语法分析
    std::cout << "\n--- Parsing --- H" << std::endl;
    Parser parser(tokens, error_handler);
    std::unique_ptr<ProgramNode> ast_root = parser.parse();

    if (error_handler.has_errors() || !ast_root) {
        error_handler.print_errors();
        std::cerr << "Compilation failed due to syntax errors." << std::endl;
        if(!ast_root && !error_handler.has_errors()){
            std::cerr << "Parser returned null AST without reporting errors through handler." << std::endl;
        }
        return 1;
    }
    std::cout << "Parsing completed. AST generated." << std::endl;
    // 输出 AST (这里需要 ProgramNode 有打印方法)
    std::cout << "--- Abstract Syntax Tree (AST) --- H" << std::endl;
    if (ast_root) {
        // 假设 ProgramNode 或其基类有 print() 或类似方法
        // ast_root->print(std::cout, 0); // 示例调用，具体方法未知
        std::cout << "AST Root: " << ast_root.get() << " (Address)" << std::endl; // 暂时打印地址
        std::cout << "(AST 打印功能需要 ProgramNode 支持具体的打印实现)" << std::endl;
    } else {
        std::cout << "AST is null." << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    // 语义分析
    std::cout << "\n--- Semantic Analysis --- H" << std::endl;
    SemanticAnalyzer semantic_analyzer(error_handler);
    semantic_analyzer.analyze(ast_root.get());

    if (error_handler.has_errors()) {
        error_handler.print_errors();
        std::cerr << "Compilation failed due to semantic errors." << std::endl;
        return 1;
    }
    std::cout << "Semantic analysis completed." << std::endl;
    // 输出语义分析结果 (例如符号表，或再次打印 AST 查看变化)
    std::cout << "--- Semantic Analysis Results --- H" << std::endl;
    // semantic_analyzer.printSymbolTable(); // 假设有此方法
    // 或者再次打印 AST
    // if (ast_root) {
    //     ast_root->print(std::cout, 0);
    // }
    std::cout << "(语义分析结果打印功能需要 SemanticAnalyzer 或 AST 支持)" << std::endl;
    std::cout << "-------------------------------" << std::endl;

    // 代码生成 (暂时注释掉，专注于前端和中间代码生成)
    /*
    std::cout << "\n--- Code Generation --- H" << std::endl;
    CodeGenerator code_generator(error_handler);
    std::string generated_code = code_generator.generate(ast_root.get());

    if (error_handler.has_errors()) {
        error_handler.print_errors();
        std::cerr << "Compilation failed during code generation." << std::endl;
        return 1;
    }
    std::cout << "Code generation completed." << std::endl;
    std::cout << "\n--- Generated Code --- H" << std::endl;
    std::cout << generated_code << std::endl;
    std::cout << "----------------------" << std::endl;
    */

    std::cout << "\nCompilation process (up to semantic analysis) finished successfully." << std::endl;

    return 0;
} 