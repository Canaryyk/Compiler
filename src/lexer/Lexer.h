#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"
#include "../utils/ErrorHandler.h"

class Lexer {
public:
    Lexer(std::string source, ErrorHandler& errorHandler);
    std::vector<Token> tokenize();

private:
    std::string source_code_;
    ErrorHandler& error_handler_;
    int current_pos_ = 0;
    int line_ = 1;
    int column_ = 1;

    char advance();
    char peek() const;
    char peek_next() const;
    bool is_at_end() const;
    bool match(char expected);

    Token make_token(TokenType type, const std::string& lexeme);
    Token make_token(TokenType type, const std::string& lexeme, const std::variant<int, double, std::string>& value);
    Token identifier();
    Token number();
    Token string_literal(); // 如果文法支持字符串字面量
    void skip_whitespace_and_comments(); // 跳过空白和注释

    // 根据文法添加关键字映射
    std::unordered_map<std::string, TokenType> keywords_;
};

#endif //LEXER_H 