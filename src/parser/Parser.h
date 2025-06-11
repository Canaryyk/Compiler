/**
 * @file Parser.h
 * @brief 定义了语法分析器 (Parser)。
 *
 * 这是一个手写的递归下降语法分析器，负责对词法分析器(Lexer)生成的
 * Token 序列进行语法分析，同时进行语义检查和中间代码（四元式）生成。
 */
#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <stack>
#include <string>
#include "../lexer/Lexer.h"
#include "../semantic_analyzer/SymbolTable.h"
#include "Quadruple.h"

/**
 * @class Parser
 * @brief 实现了递归下降语法分析、语义分析和中间代码生成。
 *
 * 该类通过一系列相互递归的函数来匹配语言的文法规则。
 * 在匹配成功时，它会调用符号表进行语义检查（如类型检查、符号声明检查），
 * 并生成一系列四元式作为程序的中间表示。
 */
class Parser {
public:
    /**
     * @brief 构造一个语法分析器。
     * @param lexer 对词法分析器的引用，用于获取 Token。
     * @param table 对符号表的引用，用于语义分析。
     */
    Parser(Lexer& lexer, SymbolTable& table);

    /**
     * @brief 启动语法分析过程。
     * 这是分析器的入口点。
     */
    void parse();

    /**
     * @brief 获取生成的四元式序列。
     * @return 一个包含所有已生成四元式的 const vector 引用。
     */
    const std::vector<Quadruple>& get_quadruples() const;

private:
    // --- 核心组件 ---
    Lexer& lexer;                     ///< 词法分析器实例的引用。
    SymbolTable& table;               ///< 符号表实例的引用。
    Token current_token;              ///< 当前正在处理的 Token。
    std::vector<Quadruple> quadruples;///< 生成的四元式序列。

    // --- 状态计数器 ---
    int temp_counter;    ///< 用于生成唯一临时变量名的计数器 (t0, t1, ...)。
    int label_counter;   ///< 用于生成唯一标签名的计数器 (L0, L1, ...)。
    int current_address; ///< 用于为新声明的变量分配内存地址（偏移量）。

    // --- 内部辅助函数 ---
    
    /**
     * @brief 向前读取一个 Token，更新 current_token。
     */
    void advance();

    /**
     * @brief 匹配并消耗一个期望的 Token。
     * 如果当前 Token 类型与期望的类型匹配，则调用 advance()；否则，抛出语法错误。
     * @param category 期望的 Token 种类。
     */
    void match(TokenCategory category);

    /**
     * @brief 创建一个新的临时变量操作数。
     * @return 一个类型为 TEMPORARY 的新操作数。
     */
    Operand new_temp();

    /**
     * @brief 生成一条新的四元式并添加到序列中。
     * @param op 操作码。
     * @param arg1 第一个操作数。
     * @param arg2 第二个操作数。
     * @param result 结果操作数。
     */
    void emit(OpCode op, const Operand& arg1, const Operand& arg2, const Operand& result);

    /**
     * @brief 回填。
     * 将之前生成的某条跳转指令的目标地址设置为当前位置。
     * @param quad_index 要回填的四元式在序列中的索引。
     * @param target_label 跳转的目标标签（通常是当前四元式的索引）。
     */
    void backpatch(int quad_index, int target_label);

    // --- 文法规则对应的递归下降函数 ---
    
    void program();             // <Program> ::= program <Identifier> <Block> .
    void block();               // <Block> ::= <Declarations> <CompoundStatement>
    void declarations();        // <Declarations> ::= [ <VarDeclarations> ] [ <SubprogramDeclarations> ]
    void var_declarations();    // <VarDeclarations> ::= var <IdentifierList> : <Type> ; ...
    
    // --- 子程序和参数处理 ---
    void subprogram_declarations(); // { <ProcedureDeclaration> | <FunctionDeclaration> }
    void procedure_declaration();   // 'procedure' <Identifier> [ '(' <ParameterList> ')' ] ';' <Block> ';'
    void function_declaration();    // 'function' <Identifier> [ '(' <ParameterList> ')' ] ':' <Type> ';' <Block> ';'
    std::unique_ptr<SubprogramInfo> parameter_list(); // <Parameter> { ';' <Parameter> }
    void parameter(SubprogramInfo& info); // <IdentifierList> ':' <Type>

    std::vector<Token> identifier_list(); // <IdentifierList> ::= <Identifier> { , <Identifier> }
    TypeEntry* type(); // <Type> ::= 'integer' | 'real'

    // --- 语句解析 ---
    void compound_statement();  // <CompoundStatement> ::= begin <StatementList> end
    void statement_list();      // <StatementList> ::= <Statement> { ; <Statement> }
    void statement();           // <Statement> ::= <AssignmentStatement> | <IfStatement> | ...
    void assignment_statement();// <AssignmentStatement> ::= <Identifier> := <Expression>
    void if_statement();        // <IfStatement> ::= if <Condition> then <Statement> [else <Statement>]
    void while_statement();     // <WhileStatement> ::= while <Condition> do <Statement>
    void print_statement();     // <PrintStatement> ::= print <Expression>
    Operand subprogram_call(SymbolEntry* symbol);   // <Identifier> '(' [ <ArgumentList> ] ')'
    
    // --- 表达式和条件解析 ---
    Operand expression();       // <Expression> ::= <Term> { (+|-) <Term> }
    Operand term();             // <Term> ::= <Factor> { (*|/) <Factor> }
    Operand factor();           // <Factor> ::= <Identifier> | <Constant> | ( <Expression> ) | <SubprogramCall>
    Operand condition();        // <Condition> ::= <Expression> <RelationalOp> <Expression>
    OpCode relational_op();     // <RelationalOp> ::= = | <> | < | <= | > | >=
};
 std::string opcode_to_string(OpCode op);
#endif // PARSER_H