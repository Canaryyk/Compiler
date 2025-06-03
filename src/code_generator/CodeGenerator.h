#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "../parser/AST.h" // 需要访问AST节点
#include "../semantic_analyzer/SymbolTable.h" // 可能需要符号表信息
#include "../utils/ErrorHandler.h"
#include <string>
#include <sstream> // 用于构建输出代码字符串

class CodeGenerator : public ASTVisitor { // 继承ASTVisitor以遍历AST
public:
    CodeGenerator(ErrorHandler& errorHandler /*, SymbolTable& symbolTable*/); // SymbolTable might not be needed directly if types are on AST or not used for basic generation
    // CodeGenerator(/*ErrorHandler& errorHandler, SymbolTable& symbolTable*/);

    std::string generate(ProgramNode* program_root);

    // --- 实现 ASTVisitor 接口 --- 
    // (根据需要覆盖访问者方法进行代码生成)
    void visit(ProgramNode* node) override;
    void visit(SubprogramNode* node) override;
    void visit(VariableDeclarationNode* node) override;
    void visit(IdentifierListNode* node) override; 
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
    ErrorHandler& error_handler_;
    // ErrorHandler& error_handler_;
    // SymbolTable& symbol_table_; // 可能需要符号表来获取变量类型等信息以生成正确的代码
    std::stringstream output_code_stream_; // 用于逐步构建生成的代码
    int label_counter_ = 0; // 用于生成唯一的标签（如果需要，例如用于跳转）

    std::string new_label();
    std::string map_type_to_target(TokenType type); // 将源语言类型映射到目标语言类型
    // ... 其他代码生成相关的辅助函数
};

#endif //CODE_GENERATOR_H 