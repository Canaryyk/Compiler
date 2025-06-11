/**
 * @file Parser.cpp
 * @brief 语法分析器的实现文件。
 *
 * 包含了 Parser 类中所有方法的具体实现，通过递归下降的方式
 * 对 Token 序列进行语法分析、语义检查和四元式中间代码生成。
 */
#include "Parser.h"
#include <iostream>
#include <stdexcept>

/**
 * @brief Parser 构造函数。
 * @param lexer 对词法分析器的引用，用于获取 Token。
 * @param table 对符号表的引用，用于符号管理和语义分析。
 */
Parser::Parser(Lexer& lexer, SymbolTable& table)
    : lexer(lexer), table(table), temp_counter(0), label_counter(0), current_address(0) {
    advance();
}

/**
 * @brief 启动语法分析过程。
 * 这是整个语法分析的入口点，它从顶层文法规则 <program> 开始。
 */
void Parser::parse() {
    program();
}

/**
 * @brief 获取分析完成后生成的四元式序列。
 * @return 一个包含所有已生成四元式的 const vector 引用。
 */
const std::vector<Quadruple>& Parser::get_quadruples() const {
    return quadruples;
}

/**
 * @brief 从词法分析器获取下一个 Token，并更新 current_token。
 * 这是分析器向前"看"一个符号的唯一方式。
 */
void Parser::advance() {
    current_token = lexer.get_next_token();
}

/**
 * @brief 匹配并消耗一个期望的 Token。
 * 这是语法分析器的核心操作之一。它检查当前 Token 是否符合特定类别，
 * 如果符合，就调用 advance() 消耗掉它，让分析继续；否则，说明出现语法错误。
 * @param category 期望匹配的 Token 种类。
 * @throws std::runtime_error 如果当前 Token 与期望不符，抛出语法错误异常。
 */
void Parser::match(TokenCategory category) {
    if (current_token.category == category) {
        advance();
    } else {
        throw std::runtime_error("Syntax error: Unexpected token.");
    }
}

/**
 * @brief 创建一个新的临时变量操作数。
 * 在处理表达式时，需要临时变量来存储中间计算结果。
 * @return 一个类型为 TEMPORARY 的新操作数。
 */
Operand Parser::new_temp() {
    std::string temp_name = "t" + std::to_string(temp_counter);
    int index = temp_counter;
    temp_counter++;
    return Operand{Operand::Type::TEMPORARY, index, temp_name};
}

/**
 * @brief 生成一条新的四元式并添加到序列的末尾。
 * 这是将分析结果转化为中间代码的关键步骤。
 * @param op 四元式的操作码。
 * @param arg1 第一个操作数。
 * @param arg2 第二个操作数。
 * @param result 结果操作数或跳转目标。
 */
void Parser::emit(OpCode op, const Operand& arg1, const Operand& arg2, const Operand& result) {
    quadruples.push_back({op, arg1, arg2, result});
}

/**
 * @brief 回填。
 * 对于跳转指令（如 if, while），在生成时其跳转目标地址是未知的。
 * 当确定了目标地址后，调用此函数将地址填回到之前生成的四元式中。
 * @param quad_index 需要回填的四元式在序列中的索引。
 * @param target_label 跳转的目标标签（通常是另一条四元式的索引）。
 */
void Parser::backpatch(int quad_index, int target_label) {
    quadruples[quad_index].result = {Operand::Type::LABEL, target_label, "L" + std::to_string(target_label)};
}

/**
 * @brief 解析 <Program> ::= program <Identifier> <Block> .
 * 这是顶层语法规则，定义了一个完整程序的结构。
 */
void Parser::program() {
    if (table.get_keyword_table()[current_token.index - 1] != "program") 
        throw std::runtime_error("Syntax error: Expected 'program'.");
    match(TokenCategory::KEYWORD);
    match(TokenCategory::IDENTIFIER);
    block();
    if (table.get_operator_table()[current_token.index - 1] != ".")
        throw std::runtime_error("Syntax error: Expected '.'.");
    match(TokenCategory::OPERATOR);
}

/**
 * @brief 解析 <Block> ::= [<VarDeclarations>] <CompoundStatement>
 * 块是程序的基本组成单元，包含可选的变量声明和一条复合语句。
 */
void Parser::block() {
    table.enter_scope();
    if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index - 1] == "var") {
        var_declarations();
    }
    compound_statement();
    table.exit_scope();
}

/**
 * @brief 解析 <VarDeclarations> ::= var <IdentifierList> : <Type> ; ...
 * 处理变量声明部分。
 */
void Parser::var_declarations() {
    match(TokenCategory::KEYWORD);
    while (current_token.category == TokenCategory::IDENTIFIER) {
        std::vector<Token> id_list = identifier_list();
        if (table.get_operator_table()[current_token.index-1] != ":") throw std::runtime_error("Syntax error: Expected ':'.");
        match(TokenCategory::OPERATOR);
        
        TypeEntry* var_type = new TypeEntry();
        std::string type_name = table.get_keyword_table()[current_token.index - 1];
        if (type_name == "integer") {
            var_type->kind = TypeKind::SIMPLE;
            var_type->size = 4;
        } else if (type_name == "real") {
            var_type->kind = TypeKind::SIMPLE;
            var_type->size = 8;
        } else {
            delete var_type;
            throw std::runtime_error("Unsupported variable type: " + type_name);
        }
        table.add_type(var_type);
        match(TokenCategory::KEYWORD);
        if (table.get_operator_table()[current_token.index-1] != ";") throw std::runtime_error("Syntax error: Expected ';'.");
        match(TokenCategory::OPERATOR);

        for (const auto& id_token : id_list) {
            SymbolEntry entry;
            entry.name = table.get_simple_identifier_table()[id_token.index - 1];
            entry.category = SymbolCategory::VARIABLE;
            entry.type = var_type;
            entry.address = current_address;
            entry.scope_level = table.get_current_scope_level();
            if (!table.add_symbol(entry)) {
                throw std::runtime_error("Semantic error: Redefinition of symbol '" + entry.name + "'.");
            }
            current_address += var_type->size;
        }
    }
}

/**
 * @brief 解析 <IdentifierList> ::= <Identifier> { , <Identifier> }
 * @return 包含所有已解析标识符 Token 的向量，用于后续的符号表操作。
 */
std::vector<Token> Parser::identifier_list() {
    std::vector<Token> id_list;
    id_list.push_back(current_token);
    match(TokenCategory::IDENTIFIER);
    while (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index - 1] == ",") {
        advance();
        id_list.push_back(current_token);
        match(TokenCategory::IDENTIFIER);
    }
    return id_list;
}

/**
 * @brief 解析 <CompoundStatement> ::= begin <StatementList> end
 * 复合语句是一个由 begin 和 end 包围的语句列表。
 */
void Parser::compound_statement() {
    if (table.get_keyword_table()[current_token.index - 1] != "begin") throw std::runtime_error("Syntax error: Expected 'begin'.");
    match(TokenCategory::KEYWORD);
    statement_list();
    if (table.get_keyword_table()[current_token.index - 1] != "end") throw std::runtime_error("Syntax error: Expected 'end'.");
    match(TokenCategory::KEYWORD);
}

/**
 * @brief 解析 <StatementList> ::= <Statement> { ; <Statement> }
 * 语句列表由一个或多个由分号分隔的语句构成。
 */
void Parser::statement_list() {
    statement();
    while (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index - 1] == ";") {
        advance();
        if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index-1] == "end") {
            break;
        }
        statement();
    }
}

/**
 * @brief 解析 <Statement> ::= <AssignmentStatement> | <IfStatement> | <WhileStatement> | <CompoundStatement> | ...
 * 语句是程序执行的基本单位。
 */
void Parser::statement() {
    if (current_token.category == TokenCategory::IDENTIFIER) {
        assignment_statement();
    } else if (current_token.category == TokenCategory::KEYWORD) {
        const auto& keyword = table.get_keyword_table()[current_token.index - 1];
        if (keyword == "if") if_statement();
        else if (keyword == "while") while_statement();
        else if (keyword == "begin") compound_statement();
    }
}

/**
 * @brief 解析 <AssignmentStatement> ::= <Identifier> := <Expression>
 */
void Parser::assignment_statement() {
    const auto& id_name = table.get_simple_identifier_table()[current_token.index - 1];
    SymbolEntry* left_entry = table.find_symbol(id_name);
    if (!left_entry) throw std::runtime_error("Semantic error: Undeclared identifier '" + id_name + "'.");
    
    Operand left = {Operand::Type::IDENTIFIER, left_entry->address, left_entry->name};
    match(TokenCategory::IDENTIFIER);
    
    if (table.get_operator_table()[current_token.index - 1] != ":=") throw std::runtime_error("Syntax error: Expected ':='.");
    match(TokenCategory::OPERATOR);
    
    Operand right = expression();
    emit(OpCode::ASSIGN, right, {}, left);
}

/**
 * @brief 解析 <IfStatement> ::= if <Condition> then <Statement> [else <Statement>]
 */
void Parser::if_statement() {
    match(TokenCategory::KEYWORD);
    Operand cond = condition();
    if (table.get_keyword_table()[current_token.index - 1] != "then") throw std::runtime_error("Syntax error: Expected 'then'.");
    match(TokenCategory::KEYWORD);

    int false_quad_idx = quadruples.size();
    emit(OpCode::JUMP_IF_FALSE, cond, {}, {});

    statement();

    if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index - 1] == "else") {
        advance();
        int exit_quad_idx = quadruples.size();
        emit(OpCode::JUMP, {}, {}, {});
        backpatch(false_quad_idx, quadruples.size());
        statement();
        backpatch(exit_quad_idx, quadruples.size());
    } else {
        backpatch(false_quad_idx, quadruples.size());
    }
}

/**
 * @brief 解析 <WhileStatement> ::= while <Condition> do <Statement>
 */
void Parser::while_statement() {
    match(TokenCategory::KEYWORD);
    int loop_start_addr = quadruples.size();
    Operand cond = condition();
    if (table.get_keyword_table()[current_token.index - 1] != "do") throw std::runtime_error("Syntax error: Expected 'do'.");
    match(TokenCategory::KEYWORD);
    
    int false_quad_idx = quadruples.size();
    emit(OpCode::JUMP_IF_FALSE, cond, {}, {});

    statement();
    
    emit(OpCode::JUMP, {}, {}, {Operand::Type::LABEL, loop_start_addr, "L" + std::to_string(loop_start_addr)});
    backpatch(false_quad_idx, quadruples.size());
}

/**
 * @brief 解析 <Expression> ::= <Term> { (+|-) <Term> }
 * 表达式由一个或多个通过加法或减法运算符连接的项构成。
 */
Operand Parser::expression() {
    Operand left = term();
    while (current_token.category == TokenCategory::OPERATOR) {
        const auto& op_str = table.get_operator_table()[current_token.index - 1];
        if (op_str == "+" || op_str == "-") {
            OpCode op = (op_str == "+") ? OpCode::ADD : OpCode::SUB;
            advance();
            Operand right = term();
            Operand result = new_temp();
            emit(op, left, right, result);
            left = result;
        } else {
            break;
        }
    }
    return left;
}

/**
 * @brief 解析 <Term> ::= <Factor> { (*|/) <Factor> }
 * 项由一个或多个通过乘法或除法运算符连接的因子构成。
 */
Operand Parser::term() {
    Operand left = factor();
    while (current_token.category == TokenCategory::OPERATOR) {
        const auto& op_str = table.get_operator_table()[current_token.index - 1];
        if (op_str == "*" || op_str == "/") {
            OpCode op = (op_str == "*") ? OpCode::MUL : OpCode::DIV;
            advance();
            Operand right = factor();
            Operand result = new_temp();
            emit(op, left, right, result);
            left = result;
        } else {
            break;
        }
    }
    return left;
}

/**
 * @brief 解析 <Factor> ::= <Identifier> | <Constant> | ( <Expression> )
 * 因子是表达式的最基本单元。
 */
Operand Parser::factor() {
    Operand op;
    if (current_token.category == TokenCategory::IDENTIFIER) {
        const auto& id_name = table.get_simple_identifier_table()[current_token.index - 1];
        SymbolEntry* entry = table.find_symbol(id_name);
        if (!entry) throw std::runtime_error("Semantic error: Undeclared identifier '" + id_name + "'.");
        op = {Operand::Type::IDENTIFIER, entry->address, entry->name};
        advance();
    } else if (current_token.category == TokenCategory::CONSTANT) {
        op = {Operand::Type::CONSTANT, current_token.index - 1};
        op.name = std::to_string(static_cast<int>(table.get_constant_table()[op.index]));
        advance();
    } else if (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index - 1] == "(") {
        advance();
        op = expression();
        if (table.get_operator_table()[current_token.index - 1] != ")") throw std::runtime_error("Syntax error: Expected ')'.");
        advance();
    } else {
        throw std::runtime_error("Syntax error in factor.");
    }
    return op;
}

/**
 * @brief 解析 <Condition> ::= <Expression> <RelationalOp> <Expression>
 * 条件是用于 if 和 while 语句的布尔表达式。
 */
Operand Parser::condition() {
    Operand left = expression();
    OpCode op = relational_op();
    Operand right = expression();
    Operand result = new_temp();
    emit(op, left, right, result);
    return result;
}

/**
 * @brief 解析 <RelationalOp> ::= = | <> | < | <= | > | >=
 * 匹配一个关系操作符并返回对应的操作码。
 */
OpCode Parser::relational_op() {
    if (current_token.category == TokenCategory::OPERATOR) {
        const auto& op_str = table.get_operator_table()[current_token.index - 1];
        OpCode op;
        if (op_str == "=") op = OpCode::EQ;
        else if (op_str == "<>") op = OpCode::NE;
        else if (op_str == "<") op = OpCode::LT;
        else if (op_str == "<=") op = OpCode::LE;
        else if (op_str == ">") op = OpCode::GT;
        else if (op_str == ">=") op = OpCode::GE;
        else throw std::runtime_error("Invalid relational operator.");
        advance();
        return op;
    }
    throw std::runtime_error("Expected relational operator.");
}