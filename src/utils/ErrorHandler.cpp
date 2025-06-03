#include "ErrorHandler.h"

ErrorHandler::ErrorHandler() {}

void ErrorHandler::report(Error::Type type, const std::string& message, int line, int column, const std::string& lexeme) {
    errors_.emplace_back(type, message, line, column, lexeme);
    // Optionally print immediately to cerr
    // std::cerr << errors_.back().toString() << std::endl;
}

void ErrorHandler::report_lexical_error(const std::string& message, int line, int column, const std::string& current_char_sequence) {
    report(Error::Type::LEXICAL, message, line, column, current_char_sequence);
}

void ErrorHandler::report_syntax_error(const Token& token, const std::string& message) {
    report(Error::Type::SYNTAX, message, token.line_number, token.column_number, token.lexeme);
}

void ErrorHandler::report_semantic_error(const Token& token, const std::string& message) {
    report(Error::Type::SEMANTIC, message, token.line_number, token.column_number, token.lexeme);
}

void ErrorHandler::report_semantic_error(int line, int column, const std::string& offending_item, const std::string& message) {
    report(Error::Type::SEMANTIC, message, line, column, offending_item);
}


bool ErrorHandler::has_errors() const {
    return !errors_.empty();
}

void ErrorHandler::print_errors() const {
    for (const auto& error : errors_) {
        std::cerr << error.toString() << std::endl;
    }
}

const std::vector<Error>& ErrorHandler::get_errors() const {
    return errors_;
}

void ErrorHandler::clear_errors(){
    errors_.clear();
} 