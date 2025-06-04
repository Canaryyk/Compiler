#include "Lexer.h"
#include <stdexcept> // For std::out_of_range
#include <cctype>    // For isalpha, isdigit
#include <iostream>  // For debugging
#include "../utils/ErrorHandler.h"

Lexer::Lexer(std::string source, ErrorHandler& errorHandler)
    : source_code_(std::move(source)), error_handler_(errorHandler) {
    keywords_["program"] = TokenType::PROGRAM;
    keywords_["var"] = TokenType::VAR;
    keywords_["begin"] = TokenType::BEGIN;
    keywords_["end"] = TokenType::END;
    keywords_["integer"] = TokenType::INTEGER;
    keywords_["real"] = TokenType::REAL;
    keywords_["char"] = TokenType::CHAR;
    // 注意：文法中的 ω0 (+, -) 和 ω1 (*, /) 需要在tokenize中单独处理
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!is_at_end()) {
        skip_whitespace_and_comments();
        if (is_at_end()) break;

        char c = advance();
        int start_column = column_ -1; // advance会增加column

        // 标识符和关键字
        if (std::isalpha(c) || c == '_') { // 假设标识符以字母或下划线开头
            std::string lexeme;
            lexeme += c;
            while (!is_at_end() && (std::isalnum(peek()) || peek() == '_')) {
                lexeme += advance();
            }
            auto it = keywords_.find(lexeme);
            if (it != keywords_.end()) {
                tokens.push_back(make_token(it->second, lexeme));
            } else {
                tokens.push_back(make_token(TokenType::IDENTIFIER, lexeme, lexeme)); // 标识符的值就是其本身
            }
            continue;
        }

        // 数字 (整数和实数)
        if (std::isdigit(c)) {
            std::string num_str;
            num_str += c;
            bool is_real = false;
            while (!is_at_end() && std::isdigit(peek())) {
                num_str += advance();
            }
            if (!is_at_end() && peek() == '.') {
                 // 检查'.'后面是否是数字，避免 program.<EOF> 被误认为实数
                if (std::isdigit(peek_next())) {
                    is_real = true;
                    num_str += advance(); // consume '.'
                    while (!is_at_end() && std::isdigit(peek())) {
                        num_str += advance();
                    }
                } 
            }
            if (is_real) {
                try {
                    tokens.push_back(make_token(TokenType::REAL_CONST, num_str, std::stod(num_str)));
                } catch (const std::out_of_range& oor) {
                    error_handler_.report_lexical_error("Real number out of range: " + num_str, line_, start_column, num_str);
                    tokens.push_back(make_token(TokenType::UNKNOWN, num_str));
                }
            } else {
                 try {
                    tokens.push_back(make_token(TokenType::INTEGER_CONST, num_str, std::stoi(num_str)));
                } catch (const std::out_of_range& oor) {
                    error_handler_.report_lexical_error("Integer number out of range: " + num_str, line_, start_column, num_str);
                    tokens.push_back(make_token(TokenType::UNKNOWN, num_str));
                }
            }
            continue;
        }

        // 字符串字面量
        if (c == '"') {
            tokens.push_back(string_literal());
            continue;
        }

        // 字符字面量 (例如: 'a', '\'\'')
        if (c == '\'') { // 单引号开始
            std::string lexeme; 
            lexeme += c; // 添加起始单引号
            char char_val = '\0';
            bool malformed = false;
            int start_char_line = line_;
            int start_char_col = column_ -1;

            if (is_at_end()) { // 文件在 ' 之后结束
                error_handler_.report_lexical_error("Unterminated character literal (EOF after opening quote).", start_char_line, start_char_col, lexeme);
                tokens.push_back(make_token(TokenType::UNKNOWN, lexeme));
                continue;
            }

            char next_char = advance();
            lexeme += next_char;

            if (next_char == '\\') { // 转义序列
                if (is_at_end()) {
                    error_handler_.report_lexical_error("Unterminated character literal (EOF after backslash).", start_char_line, start_char_col, lexeme);
                    tokens.push_back(make_token(TokenType::UNKNOWN, lexeme));
                    continue;
                }
                char escaped = advance();
                lexeme += escaped;
                switch (escaped) {
                    case '\'': char_val = '\''; break;
                    case '\\': char_val = '\\'; break;
                    case 'n': char_val = '\n'; break; // 支持 \n, \t 等转义
                    case 't': char_val = '\t'; break;
                    // case '0': char_val = '\0'; break; // 如果支持空字符
                    default:
                        error_handler_.report_lexical_error("Unknown escape sequence in character literal: \\" + std::string(1, escaped), line_, column_ - 2, "\\" + std::string(1, escaped));
                        malformed = true;
                        break;
                }
            } else if (next_char == '\'') { // 空字符字面量 '''
                 error_handler_.report_lexical_error("Empty character literal.", start_char_line, start_char_col, lexeme);
                 malformed = true;
            } else if (next_char == '\n' || next_char == '\r') { // 换行符在字符字面量中
                error_handler_.report_lexical_error("Newline in character literal.", start_char_line, start_char_col, lexeme);
                malformed = true;
                // 回退，让主循环处理换行
                current_pos_--; 
                if (next_char == '\n') { line_--; column_ = 1; /*approximate prior col for error*/ }
                 else { column_--; }
            } else {
                char_val = next_char;
            }

            if (!malformed) {
                if (is_at_end() || peek() != '\'') { // 缺少末尾单引号或者内容过多
                    if(!is_at_end() && peek() != '\'') {
                         // 尝试读取多余的字符，直到找到关闭引号或空格等，以提高错误词素的准确性
                        std::string extra_chars;
                        while(!is_at_end() && peek() != '\'' && !std::isspace(peek()) && lexeme.length() < 10) {
                            char offending_char = advance();
                            lexeme += offending_char;
                            extra_chars += offending_char;
                        }
                        error_handler_.report_lexical_error("Character literal too long or not properly closed (e.g., 'ab' or 'a). Extracted: " + extra_chars, start_char_line, start_char_col, lexeme);
                    } else {
                         error_handler_.report_lexical_error("Unterminated character literal (missing closing quote).", start_char_line, start_char_col, lexeme);
                    }
                    malformed = true;
                } else {
                    lexeme += advance(); // 消耗末尾单引号
                }
            }

            if (malformed) {
                tokens.push_back(make_token(TokenType::UNKNOWN, lexeme));
            } else {
                tokens.push_back(make_token(TokenType::CHAR_CONST, lexeme, std::string(1, char_val)));
            }
            continue;
        }

        // 运算符和分隔符
        switch (c) {
            case '+': tokens.push_back(make_token(TokenType::PLUS, "+")); break;
            case '-': tokens.push_back(make_token(TokenType::MINUS, "-")); break;
            case '*': tokens.push_back(make_token(TokenType::MULTIPLY, "*")); break;
            case '/': tokens.push_back(make_token(TokenType::DIVIDE, "/")); break;
            case ';': tokens.push_back(make_token(TokenType::SEMICOLON, ";")); break;
            case ',': tokens.push_back(make_token(TokenType::COMMA, ",")); break;
            case ':':
                if (match('=')) {
                    tokens.push_back(make_token(TokenType::ASSIGN, ":="));
                } else {
                    tokens.push_back(make_token(TokenType::COLON, ":"));
                }
                break;
            case '.': tokens.push_back(make_token(TokenType::DOT, ".")); break;
            case '(': tokens.push_back(make_token(TokenType::LPAREN, "(")); break;
            case ')': tokens.push_back(make_token(TokenType::RPAREN, ")")); break;
            default:
                error_handler_.report_lexical_error("Unexpected character: " + std::string(1, c), line_, start_column, std::string(1,c));
                tokens.push_back(make_token(TokenType::UNKNOWN, std::string(1, c)));
                break;
        }
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line_, column_));
    return tokens;
}

char Lexer::advance() {
    if (is_at_end()) return '\0';
    char current_char = source_code_[current_pos_++];
    if (current_char == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return current_char;
}

char Lexer::peek() const {
    if (is_at_end()) return '\0';
    return source_code_[current_pos_];
}

char Lexer::peek_next() const {
    if (static_cast<int>(current_pos_ + 1) >= static_cast<int>(source_code_.length())) return '\0';
    return source_code_[current_pos_ + 1];
}

bool Lexer::is_at_end() const {
    return static_cast<int>(current_pos_) >= static_cast<int>(source_code_.length());
}

bool Lexer::match(char expected) {
    if (is_at_end()) return false;
    if (source_code_[current_pos_] != expected) return false;
    current_pos_++;
    column_++; // 假设不处理换行
    return true;
}

Token Lexer::make_token(TokenType type, const std::string& lexeme) {
    return Token(type, lexeme, line_, column_ - static_cast<int>(lexeme.length()));
}

Token Lexer::make_token(TokenType type, const std::string& lexeme, const std::variant<int, double, std::string>& value) {
    return Token(type, lexeme, value, line_, column_ - static_cast<int>(lexeme.length()));
}


void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                advance(); // advance会处理line_和column_
                break;
            case '/':
                if (peek_next() == '/') { // 单行注释 //
                    while (peek() != '\n' && !is_at_end()) {
                        advance();
                    }
                    // 如果不是文件末尾，消耗换行符
                    if (!is_at_end()) advance(); 
                } else {
                    return; // 不是注释，可能是除号
                }
                break;
            case '{': // 块注释开始
                advance(); // 消耗 '{'
                while (!is_at_end() && peek() != '}') {
                    if (peek() == '\n') { // 确保正确处理块注释内的换行
                        advance(); 
                    } else {
                        advance();
                    }
                }
                if (is_at_end()) { // 未闭合的块注释
                    error_handler_.report_lexical_error("Unterminated block comment.", line_, column_);
                } else {
                    advance(); // 消耗 '}'
                }
                break;
            default:
                return;
        }
    }
}

// string_literal() 实现
Token Lexer::string_literal() {
    std::string lexeme = "\""; // Start with the opening quote
    std::string value_str;
    int start_line = line_;
    int start_col = column_ -1; // Account for the opening quote already advanced

    while (!is_at_end() && peek() != '"') {
        char current_char = advance();
        if (current_char == '\n') { // 字符串字面量不允许未转义的换行
            error_handler_.report_lexical_error("Unterminated string literal (newline encountered).", start_line, start_col, lexeme);
            // 回退到换行符之前，以便主循环可以正确处理它或报告错误
            current_pos_--; 
            column_ = 1; // Reset column, line already handled by advance then decrement
            line_--;     // Correct line number as we are "un-consuming" the newline
            return make_token(TokenType::UNKNOWN, lexeme); 
        }
        if (current_char == '\\') { // 转义字符
            lexeme += current_char; // Add the backslash to the lexeme
            if (is_at_end()) {
                error_handler_.report_lexical_error("Unterminated string literal (EOF after backslash).", start_line, start_col, lexeme);
                return make_token(TokenType::UNKNOWN, lexeme);
            }
            char escaped_char = advance();
            lexeme += escaped_char; // Add the character after backslash to the lexeme
            switch (escaped_char) {
                case '"': value_str += '"'; break;
                case '\\': value_str += '\\'; break;
                case 'n': value_str += '\n'; break; // Optional: support \n for newline in string
                case 't': value_str += '\t'; break; // Optional: support \t for tab in string
                default:
                    // 对于未知的转义序列，可以选择报错或按原样处理
                    error_handler_.report_lexical_error("Unknown escape sequence: \\" + std::string(1, escaped_char), line_, column_ - 2, "\\" + std::string(1, escaped_char));
                    value_str += '\\'; 
                    value_str += escaped_char; 
                    break;
            }
        } else {
            lexeme += current_char;
            value_str += current_char;
        }
    }

    if (is_at_end()) { // 字符串未闭合
        error_handler_.report_lexical_error("Unterminated string literal.", start_line, start_col, lexeme);
        return make_token(TokenType::UNKNOWN, lexeme);
    }

    lexeme += advance(); // 消耗末尾的引号 " 并添加到词素
    return make_token(TokenType::STRING_CONST, lexeme, value_str);
} 