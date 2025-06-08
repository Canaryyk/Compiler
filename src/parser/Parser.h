#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <stack>
#include <string>
#include "../lexer/Lexer.h"
#include "../semantic_analyzer/SymbolTable.h"
#include "Quadruple.h"

class Parser {
public:
    Parser(Lexer& lexer, SymbolTable& table);
    void parse();
    const std::vector<Quadruple>& get_quadruples() const;

private:
    Lexer& lexer;
    SymbolTable& table;
    Token current_token;
    std::vector<Quadruple> quadruples;
    int temp_counter;
    int label_counter;
    int current_address; // 用于分配变量地址

    void advance();
    void match(TokenCategory category);
    Operand new_temp();
    void emit(OpCode op, const Operand& arg1, const Operand& arg2, const Operand& result);
    void backpatch(int quad_index, int target_label);

    void program();
    void block();
    void var_declarations();
    std::vector<Token> identifier_list();
    void compound_statement();
    void statement_list();
    void statement();
    void assignment_statement();
    void if_statement();
    void while_statement();
    
    Operand expression();
    Operand term();
    Operand factor();
    Operand condition();
    OpCode relational_op();
};

#endif // PARSER_H