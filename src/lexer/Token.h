#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <variant>

// 根据您提供的文法，定义Token类型
enum class TokenType {
    // 关键字
    PROGRAM, VAR, BEGIN, END, INTEGER, REAL, CHAR,

    // 标识符
    IDENTIFIER,

    // 常数
    INTEGER_CONST, REAL_CONST, CHAR_CONST, STRING_CONST,

    // 运算符
    PLUS, MINUS, MULTIPLY, DIVIDE, ASSIGN, // + - * /

    // 分隔符
    SEMICOLON, COMMA, COLON, DOT, LPAREN, RPAREN, // ; , : . ( )

    // 文件结束符
    END_OF_FILE,

    // 未知或错误Token
    UNKNOWN
};

// Token结构体
struct Token {
    TokenType type;
    std::string lexeme; // 词素，即Token的原始文本
    std::variant<int, double, std::string> value; // Token的值 (可选，例如常数的值，标识符的名称)
    int line_number;    // Token所在行号，用于错误报告
    int column_number;  // Token所在列号，用于错误报告

    // 临时添加默认构造函数以进行诊断
    Token() : type(TokenType::UNKNOWN), lexeme(""), line_number(0), column_number(0) {}

    Token(TokenType t, std::string lex, int l, int c)
        : type(t), lexeme(std::move(lex)), line_number(l), column_number(c) {}

    Token(TokenType t, std::string lex, std::variant<int, double, std::string> val, int l, int c)
        : type(t), lexeme(std::move(lex)), value(std::move(val)), line_number(l), column_number(c) {}

    // 辅助函数，用于打印Token信息 (调试用)
    std::string toString() const {
        std::string type_str;
        switch (type) {
            case TokenType::PROGRAM: type_str = "PROGRAM"; break;
            case TokenType::VAR: type_str = "VAR"; break;
            case TokenType::BEGIN: type_str = "BEGIN"; break;
            case TokenType::END: type_str = "END"; break;
            case TokenType::INTEGER: type_str = "INTEGER"; break;
            case TokenType::REAL: type_str = "REAL"; break;
            case TokenType::CHAR: type_str = "CHAR"; break;
            case TokenType::IDENTIFIER: type_str = "IDENTIFIER"; break;
            case TokenType::INTEGER_CONST: type_str = "INTEGER_CONST"; break;
            case TokenType::REAL_CONST: type_str = "REAL_CONST"; break;
            case TokenType::CHAR_CONST: type_str = "CHAR_CONST"; break;
            case TokenType::STRING_CONST: type_str = "STRING_CONST"; break;
            case TokenType::PLUS: type_str = "PLUS"; break;
            case TokenType::MINUS: type_str = "MINUS"; break;
            case TokenType::MULTIPLY: type_str = "MULTIPLY"; break;
            case TokenType::DIVIDE: type_str = "DIVIDE"; break;
            case TokenType::ASSIGN: type_str = "ASSIGN"; break;
            case TokenType::SEMICOLON: type_str = "SEMICOLON"; break;
            case TokenType::COMMA: type_str = "COMMA"; break;
            case TokenType::COLON: type_str = "COLON"; break;
            case TokenType::DOT: type_str = "DOT"; break;
            case TokenType::LPAREN: type_str = "LPAREN"; break;
            case TokenType::RPAREN: type_str = "RPAREN"; break;
            case TokenType::END_OF_FILE: type_str = "END_OF_FILE"; break;
            default: type_str = "UNKNOWN"; break;
        }
        std::string val_str;
        if (std::holds_alternative<int>(value)) val_str = std::to_string(std::get<int>(value));
        else if (std::holds_alternative<double>(value)) val_str = std::to_string(std::get<double>(value));
        else if (std::holds_alternative<std::string>(value)) val_str = std::get<std::string>(value);
        
        return "Token( Type: " + type_str + ", Lexeme: '" + lexeme + "'" +
               (val_str.empty() ? "" : ", Value: " + val_str) +
               ", Line: " + std::to_string(line_number) +
               ", Column: " + std::to_string(column_number) + " )";
    }
};

#endif //TOKEN_H 