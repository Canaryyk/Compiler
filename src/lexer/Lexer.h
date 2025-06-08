#ifndef LEXER_H
#define LEXER_H

#include <string>
#include "../semantic_analyzer/SymbolTable.h"
#include "Token.h"
#include <vector>

class Lexer {
public:
    Lexer(const std::string& source, SymbolTable& table);
    Token get_next_token();
    const std::vector<Token>& get_all_tokens() const;

private:
    std::string source_code;
    size_t current_pos;
    SymbolTable& symbol_table;
    std::vector<Token> tokens;

    void skip_whitespace();
    Token make_token(TokenCategory category, int index);
    Token handle_identifier();
    Token handle_number();
    Token handle_operator();
};

#endif // LEXER_H
