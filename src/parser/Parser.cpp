#include "Parser.h"
#include <stdexcept> // For std::runtime_error
// #include <iostream>  // For error reporting (temporary) - Replaced by ErrorHandler
#include "../utils/ErrorHandler.h"

Parser::Parser(std::vector<Token> tokens, ErrorHandler& errorHandler)
    : tokens_(std::move(tokens)), error_handler_(errorHandler) {
    if (tokens_.empty() || tokens_.back().type != TokenType::END_OF_FILE) {
        // This case should ideally be an internal error or assertion,
        // as Lexer is expected to always provide an EOF token.
        // error_handler_.report(Error::Type::GENERAL, "Token stream missing EOF.");
        // For now, we assume Lexer behaves correctly.
    }
}

std::unique_ptr<ProgramNode> Parser::parse() {
    try {
        return parse_program();
    } catch (const std::runtime_error& /*e*/) {
        // Errors are now reported via ErrorHandler before throwing.
        // The exception is mainly for unwinding the parsing stack.
        // No need to print e.what() as it's usually a generic message from here.
        return nullptr; // Parsing failed
    }
}

// --- 辅助方法实现 ---
const Token& Parser::current_token() const {
    if (is_at_end()) return tokens_.back(); 
    return tokens_[current_token_index_];
}

const Token& Parser::peek_token(int offset) const {
    if (current_token_index_ + offset >= tokens_.size()) {
        return tokens_.back(); 
    }
    return tokens_[current_token_index_ + offset];
}

Token Parser::consume_token() {
    if (!is_at_end()) {
        Token consumed = tokens_[current_token_index_];
        current_token_index_++;
        return consumed;
    }
    return tokens_.back(); 
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        consume_token();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return type == TokenType::END_OF_FILE;
    return current_token().type == type;
}

bool Parser::check_next(TokenType type) const {
    if (is_at_end()) return false;
    if (current_token_index_ + 1 >= tokens_.size()) return type == TokenType::END_OF_FILE;
    return peek_token().type == type;
}

Token Parser::expect(TokenType type, const std::string& error_message) {
    if (check(type)) {
        return consume_token();
    }
    error_handler_.report_syntax_error(current_token(), error_message + ". Got '" + current_token().lexeme + "'");
    throw std::runtime_error("Syntax expectation failed: " + error_message); // Exception to unwind stack
}

bool Parser::is_at_end() const {
    return current_token_index_ >= tokens_.size() -1 ;
}

void Parser::report_error(const Token& token, const std::string& message) {
    error_handler_.report_syntax_error(token, message);
    // No longer throws from here directly; expect() will throw after reporting.
    // If a non-fatal error that allows recovery is reported, synchronize() should be called.
}

// --- 递归下降解析方法实现 (骨架) ---

// <程序> -> program <标识符> <分程序>.
std::unique_ptr<ProgramNode> Parser::parse_program() {
    auto node = std::make_unique<ProgramNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->program_keyword = expect(TokenType::PROGRAM, "Expected 'program' keyword");
    node->identifier = expect(TokenType::IDENTIFIER, "Expected program name (identifier)");
    node->subprogram = parse_subprogram();
    node->dot = expect(TokenType::DOT, "Expected '.' at the end of the program");
    
    if (!check(TokenType::END_OF_FILE)) {
        report_error(current_token(), "Unexpected tokens after program end.");
        // Potentially throw or synchronize if strictness is required.
    }
    return node;
}

// <分程序> -> <变量说明> <复合语句>
std::unique_ptr<SubprogramNode> Parser::parse_subprogram() {
    auto node = std::make_unique<SubprogramNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    if (check(TokenType::VAR)) {
      node->variable_declaration = parse_variable_declaration();
    } else {
        error_handler_.report_syntax_error(current_token(), "Expected 'var' for variable declarations.");
        throw std::runtime_error("Missing variable declaration block as per grammar.");
    }
    
    node->compound_statement = parse_compound_statement();
    return node;
}

// <变量说明> -> var <标识符表> : <类型> ;
std::unique_ptr<VariableDeclarationNode> Parser::parse_variable_declaration() {
    auto node = std::make_unique<VariableDeclarationNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->var_keyword = expect(TokenType::VAR, "Expected 'var' keyword");
    node->identifier_list = parse_identifier_list();
    node->colon = expect(TokenType::COLON, "Expected ':' after identifier list");
    node->type = parse_type();
    node->semicolon = expect(TokenType::SEMICOLON, "Expected ';' after type in variable declaration");
    return node;
}

// <标识符表> -> <标识符> | <标识符> , <标识符表>
std::unique_ptr<IdentifierListNode> Parser::parse_identifier_list() {
    auto node = std::make_unique<IdentifierListNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->identifiers.push_back(expect(TokenType::IDENTIFIER, "Expected an identifier"));
    while (match(TokenType::COMMA)) {
        node->identifiers.push_back(expect(TokenType::IDENTIFIER, "Expected an identifier after comma"));
    }
    return node;
}

// <类型> -> integer | real | char
std::unique_ptr<TypeNode> Parser::parse_type() {
    auto node = std::make_unique<TypeNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    if (check(TokenType::INTEGER) || check(TokenType::REAL) || check(TokenType::CHAR)) {
        node->type_token = consume_token();
    } else {
        error_handler_.report_syntax_error(current_token(), "Expected a type (integer, real, or char)");
        throw std::runtime_error("Invalid type specified.");
    }
    return node;
}

// <复合语句> -> begin <语句表> end
std::unique_ptr<CompoundStatementNode> Parser::parse_compound_statement() {
    auto node = std::make_unique<CompoundStatementNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->begin_keyword = expect(TokenType::BEGIN, "Expected 'begin' keyword");
 
    if (check(TokenType::IDENTIFIER)) { 
        node->statement_list = parse_statement_list();
    } else if (check(TokenType::END)) { 
        node->statement_list = std::make_unique<StatementListNode>(); 
        // Note:文法 <语句表> -> <赋值语句> ; <语句表> | <赋值语句> 不允许空语句表。
        // 若要允许空 begin...end, 文法需改为 <语句表> -> <赋值语句> ; <语句表> | <赋值语句> | epsilon
        // 暂时允许空语句表，以便更灵活，但这偏离了严格的文法。
        // error_handler_.report_syntax_error(node->begin_keyword, "Empty statement list (between begin/end) is not strictly allowed by the current grammar for <语句表>.");
    } else {
         error_handler_.report_syntax_error(current_token(), "Expected statements (identifier) or 'end' after 'begin'.");
         throw std::runtime_error("Invalid start of statement list or missing 'end'.");
    }

    node->end_keyword = expect(TokenType::END, "Expected 'end' keyword");
    return node;
}

// <语句表> -> <赋值语句> ; <语句表> | <赋值语句>
// 改写为: <语句表> -> <赋值语句> { ; <赋值语句> }*
std::unique_ptr<StatementListNode> Parser::parse_statement_list() {
    auto node = std::make_unique<StatementListNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->statements.push_back(parse_assignment_statement());

    while (match(TokenType::SEMICOLON)) {
        // 如果分号后是END，则认为是合法的末尾分号 (常见情况)
        if (check(TokenType::END)) {
            break; 
        }
        // 否则，分号后必须是另一个赋值语句
        if (check(TokenType::IDENTIFIER)) {
            node->statements.push_back(parse_assignment_statement());
        } else {
             error_handler_.report_syntax_error(current_token(), "Expected an assignment statement (identifier) after ';' or 'end' keyword.");
             throw std::runtime_error("Missing statement after semicolon in statement list, or unexpected token.");
        }
    }
    return node;
}

// <赋值语句> -> <标识符> := <算术表达式>
std::unique_ptr<AssignmentStatementNode> Parser::parse_assignment_statement() {
    auto node = std::make_unique<AssignmentStatementNode>();
    node->line_number = current_token().line_number;
    node->column_number = current_token().column_number;

    node->identifier = expect(TokenType::IDENTIFIER, "Expected identifier in assignment statement");
    node->assign_op = expect(TokenType::ASSIGN, "Expected ':=' in assignment statement");
    node->expression = parse_arithmetic_expression();
    return node;
}


std::unique_ptr<ArithmeticExpressionNode> Parser::parse_arithmetic_expression() {
    auto left_term_node = parse_term();
    auto current_expr_node = std::make_unique<ArithmeticExpressionNode>(std::move(left_term_node));
    current_expr_node->line_number = current_token().line_number; 
    current_expr_node->column_number = current_token().column_number;

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = consume_token();
        auto right_term_node = parse_term();
        current_expr_node = std::make_unique<ArithmeticExpressionNode>(std::move(current_expr_node), op, std::move(right_term_node));
        current_expr_node->line_number = op.line_number;
        current_expr_node->column_number = op.column_number;
    }
    return current_expr_node;
}

std::unique_ptr<TermNode> Parser::parse_term() {
    auto left_factor_node = parse_factor();
    auto current_term_node = std::make_unique<TermNode>(std::move(left_factor_node));
    current_term_node->line_number = current_token().line_number;
    current_term_node->column_number = current_token().column_number;

    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE)) {
        Token op = consume_token();
        auto right_factor_node = parse_factor();
        current_term_node = std::make_unique<TermNode>(std::move(current_term_node), op, std::move(right_factor_node));
        current_term_node->line_number = op.line_number;
        current_term_node->column_number = op.column_number;
    }
    return current_term_node;
}

std::unique_ptr<FactorNode> Parser::parse_factor() {
    Token first_token = current_token();

    if (check(TokenType::IDENTIFIER)) {
        Token id_token = consume_token();
        auto var_node = std::make_unique<VariableNode>(id_token);
        var_node->line_number = id_token.line_number;
        var_node->column_number = id_token.column_number;
        auto factor_node = std::make_unique<FactorNode>(std::move(var_node));
        factor_node->line_number = id_token.line_number;
        factor_node->column_number = id_token.column_number;
        return factor_node;
    } else if (check(TokenType::INTEGER_CONST) || check(TokenType::REAL_CONST)) {
        Token const_token = consume_token();
        auto const_node = std::make_unique<ConstantNode>(const_token);
        const_node->line_number = const_token.line_number;
        const_node->column_number = const_token.column_number;
        auto factor_node = std::make_unique<FactorNode>(std::move(const_node));
        factor_node->line_number = const_token.line_number;
        factor_node->column_number = const_token.column_number;
        return factor_node;
    } else if (match(TokenType::LPAREN)) {
        auto expr_node = parse_arithmetic_expression();
        expect(TokenType::RPAREN, "Expected ')' after expression in factor");
        auto factor_node = std::make_unique<FactorNode>(std::move(expr_node), true );
        factor_node->line_number = first_token.line_number; 
        factor_node->column_number = first_token.column_number;
        return factor_node;
    } else {
        error_handler_.report_syntax_error(current_token(), "Expected identifier, constant, or '(' in factor.");
        throw std::runtime_error("Invalid factor.");
    }
}

void Parser::synchronize() {
    consume_token(); 
    while (!is_at_end()) {
        if (current_token().type == TokenType::SEMICOLON) {
            consume_token(); 
            return;
        }
        switch (current_token().type) {
            case TokenType::VAR:
            case TokenType::BEGIN:
            case TokenType::END:
            case TokenType::PROGRAM: 
                return;
            default:
                break; 
        }
        consume_token();
    }
} 