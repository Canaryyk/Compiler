#include<string>
#include<fstream>
#include<iostream>
#include<sstream>
// Token 类型枚举
enum class TokenType {
    IDENTIFIER,        // 标识符
    KEYWORD_PROGRAM,   // program   /关键字
    KEYWORD_VAR,       // var          
    KEYWORD_INTEGER,   // integer    
    KEYWORD_REAL,      // real     
    KEYWORD_CHAR,      // char      /
    KEYWORD_BEGIN,     // begin
    KEYWORD_END,       // end
    KEYWORD_IF,        // if         
    KEYWORD_WHILE,     // while      /
    ASSIGN_OP,         // :=         /界符
    OPERATOR,          // + - * /    /
    SEPARATOR,         // , ; :      /
    PARENTHESIS,       // ( )        /
    INTEGER_LITERAL,   // 整数字面量 /常量
    END_OF_FILE,
    UNKNOWN
};
struct Token {
    TokenType type;
    std::string value;
    int line;  // 行号，用于报错定位
};

class Lexer {
public:
    Lexer(const std::string& input) : input(input), pos(0), line(1) {}

    Token getNextToken() {
        skipWhitespace();
        if (pos >= input.length()) {
            return {TokenType::END_OF_FILE, "", line};
        }

        char currentChar = input[pos];

        // 关键字和标识符识别
        // 修改 Lexer.cpp 或 lexer.h 中的这部分代码
        if (isalpha(currentChar)) {
            std::string identifier;
            while (pos < input.length() && (isalnum(input[pos]) || input[pos] == '_')) {
                identifier += input[pos++];
            }

            if (identifier == "program") return {TokenType::KEYWORD_PROGRAM, identifier, line};
            else if (identifier == "var") return {TokenType::KEYWORD_VAR, identifier, line};
            else if (identifier == "integer") return {TokenType::KEYWORD_INTEGER, identifier, line};
            else if (identifier == "real") return {TokenType::KEYWORD_REAL, identifier, line};
            else if (identifier == "char") return {TokenType::KEYWORD_CHAR, identifier, line};
            else if (identifier == "begin") return {TokenType::KEYWORD_BEGIN, identifier, line};
            else if (identifier == "end") return {TokenType::KEYWORD_END, identifier, line};
            else if (identifier == "if") return {TokenType::KEYWORD_IF, identifier, line};    
            else if (identifier == "while") return {TokenType::KEYWORD_WHILE, identifier, line}; 
            else return {TokenType::IDENTIFIER, identifier, line};
        }

        // 数字常量识别
        if (isdigit(currentChar)) {
            std::string number;
            while (pos < input.length() && isdigit(input[pos])) {
                number += input[pos++];
            }
            return {TokenType::INTEGER_LITERAL, number, line};
        }

        // 运算符和分隔符识别
        switch (currentChar) {
            case ':':
                ++pos;
                if (pos < input.length() && input[pos] == '=') {
                    ++pos;
                    return {TokenType::ASSIGN_OP, ":=", line};
                }
                return {TokenType::SEPARATOR, ":", line};

            case ';': ++pos; return {TokenType::SEPARATOR, ";", line};
            case ',': ++pos; return {TokenType::SEPARATOR, ",", line};
            case '(': ++pos; return {TokenType::PARENTHESIS, "(", line};
            case ')': ++pos; return {TokenType::PARENTHESIS, ")", line};
            case '+': case '-': case '*': case '/':
                ++pos;
                return {TokenType::OPERATOR, std::string(1, currentChar), line};
            default:
                ++pos;  // 跳过无法识别的字符
                return {TokenType::UNKNOWN, std::string(1, currentChar), line};
        }
    }

private:
    void skipWhitespace() {
        while (pos < input.length() && isspace(input[pos])) {
            if (input[pos] == '\n') ++line;
            ++pos;
        }
    }

    std::string input;
    size_t pos;
    int line;
};
std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "Identifier";
        case TokenType::KEYWORD_PROGRAM:
        case TokenType::KEYWORD_VAR:
        case TokenType::KEYWORD_INTEGER:
        case TokenType::KEYWORD_REAL:
        case TokenType::KEYWORD_CHAR:
        case TokenType::KEYWORD_BEGIN:
        case TokenType::KEYWORD_END:
        case TokenType::KEYWORD_IF:
        case TokenType::KEYWORD_WHILE:
            return "Keyword";  // 所有关键字统一归类为 Keyword
        case TokenType::ASSIGN_OP:
        case TokenType::OPERATOR:
            return "Operator";
        case TokenType::SEPARATOR:
            return "Separator";
        case TokenType::PARENTHESIS:
            return "Parenthesis";
        case TokenType::INTEGER_LITERAL:
            return "Constant";
        case TokenType::END_OF_FILE:
            return "EOF";
        default:
            return "Unknown";
    }
}
void initLexer() {
    // 读取源文件内容
    std::ifstream file("test.pas");  // 假设 test.pas 是你的 PL/0 源文件
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();

    // 初始化词法分析器
    Lexer lexer(input);
    Token token;
    do {
        token = lexer.getNextToken();
        std::cout << "Line " << token.line << ": "
                << "Category: " << tokenTypeToString(token.type)
                << ", Value: " << token.value << std::endl;
    } while (token.type != TokenType::END_OF_FILE);
}
