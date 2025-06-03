#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include "../lexer/Token.h"
#include "AST.h"
#include "../utils/ErrorHandler.h"

class Parser {
public:
    Parser(std::vector<Token> tokens, ErrorHandler& errorHandler);
    // Parser(std::vector<Token> tokens /*, ErrorHandler& errorHandler*/);
    std::unique_ptr<ProgramNode> parse(); // 解析入口，返回程序根节点

private:
    std::vector<Token> tokens_;
    ErrorHandler& error_handler_;
    size_t current_token_index_ = 0;

    // --- 辅助方法 ---
    const Token& current_token() const;
    const Token& peek_token(int offset = 1) const; // 查看下一个token
    Token consume_token(); // 消耗当前token并前进
    bool match(TokenType type); // 检查当前token类型是否匹配，如果匹配则消耗
    bool check(TokenType type) const; // 仅检查当前token类型
    bool check_next(TokenType type) const; // 仅检查下一个token类型
    Token expect(TokenType type, const std::string& error_message); //期望特定token，否则报错
    bool is_at_end() const;

    // --- 递归下降解析方法 (根据文法产生式) ---
    // 每个非终结符对应一个解析方法
    std::unique_ptr<ProgramNode> parse_program();
    std::unique_ptr<SubprogramNode> parse_subprogram();
    std::unique_ptr<VariableDeclarationNode> parse_variable_declaration();
    std::unique_ptr<IdentifierListNode> parse_identifier_list();
    std::unique_ptr<TypeNode> parse_type();
    std::unique_ptr<CompoundStatementNode> parse_compound_statement();
    std::unique_ptr<StatementListNode> parse_statement_list();
    std::unique_ptr<AssignmentStatementNode> parse_assignment_statement();
    
    // 表达式解析 (通常比较复杂，需要处理优先级和结合性)
    // <算术表达式> -> <项> { (+|-) <项> }* (左结合)
    std::unique_ptr<ArithmeticExpressionNode> parse_arithmetic_expression();
    // <项> -> <因子> { (*|/) <因子> }* (左结合)
    std::unique_ptr<TermNode> parse_term();
    // <因子> -> <标识符> | <常数> | ( <算术表达式> )
    std::unique_ptr<FactorNode> parse_factor();
    // <算术量> (在因子中处理)
    // std::unique_ptr<ASTNode> parse_arithmetic_quantity(); 

    // 错误处理辅助
    void report_error(const Token& token, const std::string& message);
    void synchronize(); // 错误恢复同步
};

#endif //PARSER_H 