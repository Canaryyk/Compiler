#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <iostream> // For std::cerr
#include "../lexer/Token.h" // For Token information in error messages

struct Error {
    enum class Type {
        LEXICAL,
        SYNTAX,
        SEMANTIC,
        CODEGEN,
        GENERAL
    };

    Type type;
    std::string message;
    int line_number = -1;
    int column_number = -1;
    std::string offending_token_lexeme;

    Error(Type t, std::string msg, int line = -1, int col = -1, std::string lexeme = "")
        : type(t), message(std::move(msg)), line_number(line), column_number(col), offending_token_lexeme(std::move(lexeme)) {}

    std::string toString() const {
        std::string type_str;
        switch (type) {
            case Type::LEXICAL: type_str = "Lexical Error"; break;
            case Type::SYNTAX: type_str = "Syntax Error"; break;
            case Type::SEMANTIC: type_str = "Semantic Error"; break;
            case Type::CODEGEN: type_str = "Code Generation Error"; break;
            default: type_str = "General Error"; break;
        }
        std::string location_info;
        if (line_number != -1) {
            location_info = " (Line: " + std::to_string(line_number);
            if (column_number != -1) {
                location_info += ", Column: " + std::to_string(column_number);
            }
            if (!offending_token_lexeme.empty()) {
                location_info += ", Near: '" + offending_token_lexeme + "'";
            }
            location_info += ")";
        }
        return type_str + ": " + message + location_info;
    }
};

class ErrorHandler {
public:
    ErrorHandler();

    void report(Error::Type type, const std::string& message, int line = -1, int column = -1, const std::string& lexeme = "");
    void report_lexical_error(const std::string& message, int line, int column, const std::string& current_char_sequence = "");
    void report_syntax_error(const Token& token, const std::string& message);
    void report_semantic_error(const Token& token, const std::string& message); // Or pass ASTNode*
    void report_semantic_error(int line, int column, const std::string& offending_item, const std::string& message);
    // Add more specific report functions as needed for codegen etc.

    bool has_errors() const;
    void print_errors() const; // Print all collected errors
    const std::vector<Error>& get_errors() const; 
    void clear_errors();

private:
    std::vector<Error> errors_;
};

#endif //ERROR_HANDLER_H 