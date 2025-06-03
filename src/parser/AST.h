#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <memory>
#include "../lexer/Token.h" // For Token, TokenType

// 前向声明
struct ProgramNode;
struct SubprogramNode;
struct VariableDeclarationNode;
struct IdentifierListNode;
struct TypeNode;
struct CompoundStatementNode;
struct StatementListNode;
struct AssignmentStatementNode;
struct ArithmeticExpressionNode;
struct TermNode;
struct FactorNode;
struct VariableNode;       // <算术量> -> <标识符>
struct ConstantNode;       // <算术量> -> <常数>

// AST 节点访问者接口 (可选, 用于遍历AST)
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(ProgramNode* node) = 0;
    virtual void visit(SubprogramNode* node) = 0;
    virtual void visit(VariableDeclarationNode* node) = 0;
    virtual void visit(IdentifierListNode* node) = 0;
    virtual void visit(TypeNode* node) = 0;
    virtual void visit(CompoundStatementNode* node) = 0;
    virtual void visit(StatementListNode* node) = 0;
    virtual void visit(AssignmentStatementNode* node) = 0;
    virtual void visit(ArithmeticExpressionNode* node) = 0;
    virtual void visit(TermNode* node) = 0;
    virtual void visit(FactorNode* node) = 0;
    virtual void visit(VariableNode* node) = 0;
    virtual void visit(ConstantNode* node) = 0;
    // 可以为每个具体的AST节点类型添加visit方法
};

// AST 节点基类
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor* visitor) = 0; // 用于访问者模式
    // 可以添加其他通用属性，如位置信息等
    int line_number = 0;
    int column_number = 0;
};

// <程序> -> program <标识符> <分程序>.
struct ProgramNode : ASTNode {
    Token program_keyword;
    Token identifier;
    std::unique_ptr<SubprogramNode> subprogram;
    Token dot;

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <分程序> -> <变量说明> <复合语句>
struct SubprogramNode : ASTNode {
    std::unique_ptr<VariableDeclarationNode> variable_declaration; // 可能为空
    std::unique_ptr<CompoundStatementNode> compound_statement;

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <变量说明> -> var <标识符表> : <类型> ;
struct VariableDeclarationNode : ASTNode {
    Token var_keyword;
    std::unique_ptr<IdentifierListNode> identifier_list;
    Token colon;
    std::unique_ptr<TypeNode> type;
    Token semicolon;

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <标识符表> -> <标识符> , <标识符表> | <标识符>
struct IdentifierListNode : ASTNode {
    std::vector<Token> identifiers; // 存储所有标识符Token

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <类型> -> integer | real | char
struct TypeNode : ASTNode {
    Token type_token; // INTEGER, REAL, or CHAR

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <复合语句> -> begin <语句表> end
struct CompoundStatementNode : ASTNode {
    Token begin_keyword;
    std::unique_ptr<StatementListNode> statement_list;
    Token end_keyword;

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <语句表> -> <赋值语句> ; <语句表> | <赋值语句>
struct StatementListNode : ASTNode {
    std::vector<std::unique_ptr<AssignmentStatementNode>> statements;
    std::vector<Token> semicolons; // 如果需要保留分号信息

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <赋值语句> -> <标识符> := <算术表达式>
struct AssignmentStatementNode : ASTNode {
    Token identifier;
    Token assign_op; // :=
    std::unique_ptr<ArithmeticExpressionNode> expression;

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <算术表达式> -> <算术表达式> ω0 <项> | <项>
// ω0 -> + | -
struct ArithmeticExpressionNode : ASTNode {
    std::unique_ptr<ASTNode> left; // Can be ArithmeticExpressionNode or TermNode
    Token op;                      // PLUS or MINUS (optional, if not present, it's just a TermNode)
    std::unique_ptr<TermNode> right; // Only if op is present

    // 单独的 <项>
    explicit ArithmeticExpressionNode(std::unique_ptr<TermNode> term);
    // <算术表达式> ω0 <项>
    ArithmeticExpressionNode(std::unique_ptr<ArithmeticExpressionNode> expr, Token op_token, std::unique_ptr<TermNode> term);

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <项> -> <项> ω1 <因子> | <因子>
// ω1 -> * | /
struct TermNode : ASTNode {
    std::unique_ptr<ASTNode> left; // Can be TermNode or FactorNode
    Token op;                      // MULTIPLY or DIVIDE (optional)
    std::unique_ptr<FactorNode> right; // Only if op is present

    // 单独的 <因子>
    explicit TermNode(std::unique_ptr<FactorNode> factor);
    // <项> ω1 <因子>
    TermNode(std::unique_ptr<TermNode> term, Token op_token, std::unique_ptr<FactorNode> factor);

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <因子> -> <算术量> | ( <算术表达式> )
struct FactorNode : ASTNode {
    // 可能是以下之一：
    std::unique_ptr<ASTNode> content; // VariableNode, ConstantNode, or ArithmeticExpressionNode
    bool is_parenthesized = false;     // 标记是否是 ( <算术表达式> )

    explicit FactorNode(std::unique_ptr<ASTNode> cont, bool parenthesized = false);

    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <算术量> -> <标识符>
struct VariableNode : ASTNode {
    Token identifier;

    explicit VariableNode(Token id_token);
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};

// <算术量> -> <常数>
struct ConstantNode : ASTNode {
    Token constant_token; // INTEGER_CONST or REAL_CONST

    explicit ConstantNode(Token const_tok);
    void accept(ASTVisitor* visitor) override { visitor->visit(this); }
};


// --- 实现构造函数 ---
inline ArithmeticExpressionNode::ArithmeticExpressionNode(std::unique_ptr<TermNode> term)
    : left(std::move(term)), op(TokenType::UNKNOWN, "", 0, 0), right(nullptr) {}

inline ArithmeticExpressionNode::ArithmeticExpressionNode(std::unique_ptr<ArithmeticExpressionNode> expr, Token op_token, std::unique_ptr<TermNode> term)
    : left(std::move(expr)), op(std::move(op_token)), right(std::move(term)) {}

inline TermNode::TermNode(std::unique_ptr<FactorNode> factor)
    : left(std::move(factor)), op(TokenType::UNKNOWN, "", 0, 0), right(nullptr) {}

inline TermNode::TermNode(std::unique_ptr<TermNode> term, Token op_token, std::unique_ptr<FactorNode> factor)
    : left(std::move(term)), op(std::move(op_token)), right(std::move(factor)) {}

inline FactorNode::FactorNode(std::unique_ptr<ASTNode> cont, bool parenthesized)
    : content(std::move(cont)), is_parenthesized(parenthesized) {}

inline VariableNode::VariableNode(Token id_token)
    : identifier(std::move(id_token)) {}

inline ConstantNode::ConstantNode(Token const_tok)
    : constant_token(std::move(const_tok)) {}


#endif //AST_H 