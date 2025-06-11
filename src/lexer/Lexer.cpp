#include "Lexer.h"
#include <cctype>

Lexer::Lexer(const std::string& source, SymbolTable& table)
    : source_code(source), current_pos(0), symbol_table(table) {}

const std::vector<Token>& Lexer::get_all_tokens() {
    if (tokens.empty() && !source_code.empty()) {
        while(get_next_token().category != TokenCategory::END_OF_FILE);
    }
    return tokens;
}

Token Lexer::get_next_token() {
    skip_whitespace();

    if (current_pos >= source_code.length()) {
        return make_token(TokenCategory::END_OF_FILE, 0);
    }

    char current_char = source_code[current_pos];

    if (isalpha(current_char) || current_char == '_') {
        return handle_identifier();
    }

    if (isdigit(current_char)) {
        return handle_number();
    }

    return handle_operator();
}

void Lexer::skip_whitespace() {
    while (current_pos < source_code.length()) {
        if (isspace(source_code[current_pos])) {
            current_pos++;
        } else if (current_pos + 1 < source_code.length() && source_code[current_pos] == '/' && source_code[current_pos + 1] == '/') {
            // 跳过单行注释
            while (current_pos < source_code.length() && source_code[current_pos] != '\n') {
                current_pos++;
            }
        } else {
            break;
        }
    }
}

Token Lexer::make_token(TokenCategory category, int index) {
    Token token = {category, index};
    tokens.push_back(token);
    return token;
}

Token Lexer::handle_identifier() {
    size_t start_pos = current_pos;
    while (current_pos < source_code.length() && (isalnum(source_code[current_pos]) || source_code[current_pos] == '_')) {
        current_pos++;
    }
    std::string name = source_code.substr(start_pos, current_pos - start_pos);

    int keyword_index = symbol_table.find_keyword(name);
    if (keyword_index != -1) {
        return make_token(TokenCategory::KEYWORD, keyword_index + 1);
    }

    int identifier_index = symbol_table.add_identifier_for_lexer(name);
    return make_token(TokenCategory::IDENTIFIER, identifier_index + 1);
}

Token Lexer::handle_number() {
    size_t start_pos = current_pos;
    while (current_pos < source_code.length() && isdigit(source_code[current_pos])) {
        current_pos++;
    }

    if (current_pos < source_code.length() && source_code[current_pos] == '.') {
        current_pos++;
        while (current_pos < source_code.length() && isdigit(source_code[current_pos])) {
            current_pos++;
        }
    }
    
    std::string value_str = source_code.substr(start_pos, current_pos - start_pos);
    double value = std::stod(value_str);
    int constant_index = symbol_table.lookup_or_add_constant(value);
    return make_token(TokenCategory::CONSTANT, constant_index + 1);
}

Token Lexer::handle_operator() {
    // 检查双字符运算符
    if (current_pos + 1 < source_code.length()) {
        std::string two_char_op = source_code.substr(current_pos, 2);
        int op_index = symbol_table.find_operator(two_char_op);
        if (op_index != -1) {
            current_pos += 2;
            return make_token(TokenCategory::OPERATOR, op_index + 1);
        }
    }

    // 检查单字符运算符
    std::string one_char_op = source_code.substr(current_pos, 1);
    int op_index = symbol_table.find_operator(one_char_op);
    if (op_index != -1) {
        current_pos += 1;
        return make_token(TokenCategory::OPERATOR, op_index + 1);
    }

    // 未知字符
    current_pos++;
    return make_token(TokenCategory::UNKNOWN, 0);
}
