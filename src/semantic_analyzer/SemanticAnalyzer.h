#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../parser/AST.h" // 需要访问AST节点
#include "SymbolTable.h"
#include "../utils/ErrorHandler.h"

class SemanticAnalyzer : public ASTVisitor { // 继承ASTVisitor以遍历AST
public:
    SemanticAnalyzer(ErrorHandler& errorHandler);
    // SemanticAnalyzer(/*ErrorHandler& errorHandler*/);

    void analyze(ProgramNode* program_root);

    // --- 实现 ASTVisitor 接口 --- 
    // (根据需要覆盖访问者方法进行语义检查)
    void visit(ProgramNode* node) override;
    void visit(SubprogramNode* node) override;
    void visit(VariableDeclarationNode* node) override;
    void visit(IdentifierListNode* node) override; // 可能不需要直接访问这个列表节点
    void visit(TypeNode* node) override;
    void visit(CompoundStatementNode* node) override;
    void visit(StatementListNode* node) override;
    void visit(AssignmentStatementNode* node) override;
    void visit(ArithmeticExpressionNode* node) override;
    void visit(TermNode* node) override;
    void visit(FactorNode* node) override;
    void visit(VariableNode* node) override;
    void visit(ConstantNode* node) override;

private:
    SymbolTable symbol_table_;
    ErrorHandler& error_handler_;
    // ErrorHandler& error_handler_;

    // 辅助函数，例如类型检查等
    TokenType get_expression_type(ArithmeticExpressionNode* expr_node); // 获取表达式类型
    // ... 其他语义检查相关的辅助函数
};

#endif //SEMANTIC_ANALYZER_H 