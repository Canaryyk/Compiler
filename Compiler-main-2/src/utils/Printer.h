#ifndef PRINTER_H
#define PRINTER_H

#include <vector>
#include "../lexer/Token.h"
#include "../semantic_analyzer/SymbolTable.h"
#include "../parser/Quadruple.h"

class Printer {
public:
    // 打印词法分析结果
    static void print_lexical_output(const std::vector<Token>& tokens, SymbolTable& symbol_table);

    // 打印语义分析和中间代码生成结果
    static void print_semantic_output(const std::vector<Quadruple>& quadruples, SymbolTable& symbol_table);
};

#endif // PRINTER_H 