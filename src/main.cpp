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

    // 代码生成
    std::cout << "\n--- Code Generation --- H" << std::endl;
    CodeGenerator code_generator(error_handler /*, semantic_analyzer.symbol_table_*/); // semantic_analyzer.symbol_table_ is private
                                                                                      // CodeGenerator can have its own SymbolTable or receive one if needed for generation
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

    std::cout << "\nCompilation process finished successfully." << std::endl;

    return 0;
} 