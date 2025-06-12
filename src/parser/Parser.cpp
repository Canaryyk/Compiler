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
#include <memory>
#include "../semantic_analyzer/SymbolTable.h"
#include "Quadruple.h"

// opCode_to_string 函数的定义
std::string opcode_to_string(OpCode op) {
    switch(op) {
        case OpCode::ADD: return "+";
        case OpCode::SUB: return "-";
        case OpCode::MUL: return "*";
        case OpCode::DIV: return "/";
        case OpCode::ASSIGN: return ":=";
        case OpCode::EQ: return "=";
        case OpCode::NE: return "<>";
        case OpCode::LT: return "<";
        case OpCode::LE: return "<=";
        case OpCode::GT: return ">";
        case OpCode::GE: return ">=";
        case OpCode::JMP: return "j";
        case OpCode::JPF: return "j<";
        case OpCode::CALL: return "call";
        case OpCode::PARAM: return "param";
        case OpCode::RETURN: return "ret";
        case OpCode::PRINT: return "print";
        case OpCode::LABEL: return "label";
        default: return "op?";
    }
}

void print_operand_details(const std::string& label, const Operand& op) {
    std::cout << label << ": Type=" << static_cast<int>(op.type)
              << ", Index=" << op.index
              << ", Name='" << op.name << "'" << std::endl;
}

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
        // More descriptive error messages can be added here
        throw std::runtime_error("语法错误：意外的标记。");
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
        throw std::runtime_error("语法错误：应为 'program'。");
    match(TokenCategory::KEYWORD); // 'program'
    match(TokenCategory::IDENTIFIER);
    block();
    if (table.get_operator_table()[current_token.index - 1] != ".")
        throw std::runtime_error("语法错误：应为 '.'。");
    match(TokenCategory::OPERATOR); // '.'
}

/**
 * @brief 解析 <Block> ::= <Declarations> <CompoundStatement>
 */
void Parser::block() {
    table.enter_scope();
    declarations();
    compound_statement();
    table.exit_scope();
}

/**
 * @brief 解析 <Declarations> ::= [ <VarDeclarations> ] [ <SubprogramDeclarations> ]
 */
void Parser::declarations() {
    if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index - 1] == "var") {
        var_declarations();
    }
    if (current_token.category == TokenCategory::KEYWORD && 
        (table.get_keyword_table()[current_token.index - 1] == "procedure" || table.get_keyword_table()[current_token.index - 1] == "function")) {
        subprogram_declarations();
    }
}

/**
 * @brief 解析 <VarDeclarations> ::= var <VarDeclaration> { <VarDeclaration> }
 */
void Parser::var_declarations() {
    match(TokenCategory::KEYWORD); // 'var'
    while (current_token.category == TokenCategory::IDENTIFIER) {
        std::vector<Token> id_list = identifier_list();
        match(TokenCategory::OPERATOR); // ':'
        TypeEntry* var_type_ptr = type();
        match(TokenCategory::OPERATOR); // ';'

        for (const auto& id_token : id_list) {
            SymbolEntry entry;
            entry.name = table.get_simple_identifier_table()[id_token.index - 1];
            entry.category = SymbolCategory::VARIABLE;
            entry.type = var_type_ptr;
            entry.address = current_address;
            entry.scope_level = table.get_current_scope_level();
            if (!table.add_symbol(entry)) {
                throw std::runtime_error("语义错误：符号 '" + entry.name + "' 重复定义。");
            }
            current_address += var_type_ptr->size;
        }
    }
}

/**
 * @brief 解析 <SubprogramDeclarations> ::= { <ProcedureDeclaration> | <FunctionDeclaration> }
 */
void Parser::subprogram_declarations() {
    while (current_token.category == TokenCategory::KEYWORD) {
        const auto& keyword = table.get_keyword_table()[current_token.index-1];
        if (keyword == "procedure") {
            procedure_declaration();
            match(TokenCategory::OPERATOR); // ';'
        } else if (keyword == "function") {
            function_declaration();
            match(TokenCategory::OPERATOR); // ';'
        } else {
            break;
        }
    }
}

/**
 * @brief 解析 <ProcedureDeclaration>
 */
void Parser::procedure_declaration() {
    match(TokenCategory::KEYWORD); // 'procedure'
    
    SymbolEntry entry;
    entry.name = table.get_simple_identifier_table()[current_token.index - 1];
    entry.category = SymbolCategory::PROCEDURE;
    entry.type = nullptr; // Procedures have no return type
    entry.scope_level = table.get_current_scope_level();
    entry.address = quadruples.size(); // Entry point
    match(TokenCategory::IDENTIFIER);

    if (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index-1] == "(") {
        entry.subprogram_info = parameter_list();
    } else {
        entry.subprogram_info = std::make_unique<SubprogramInfo>(); // No parameters
    }

    if (!table.add_symbol(entry)) {
        throw std::runtime_error("语义错误：符号 '" + entry.name + "' 重复定义。");
    }
    
    match(TokenCategory::OPERATOR); // ';'
    block();
}

/**
 * @brief 解析 <FunctionDeclaration>
 */
void Parser::function_declaration() {
    match(TokenCategory::KEYWORD); // 'function'

    SymbolEntry entry;
    entry.name = table.get_simple_identifier_table()[current_token.index - 1];
    entry.category = SymbolCategory::FUNCTION;
    entry.scope_level = table.get_current_scope_level();
    entry.address = quadruples.size(); // Entry point
    match(TokenCategory::IDENTIFIER);

    if (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index-1] == "(") {
        entry.subprogram_info = parameter_list();
    } else {
        entry.subprogram_info = std::make_unique<SubprogramInfo>(); // No parameters
    }
    
    match(TokenCategory::OPERATOR); // ':'
    entry.type = type(); // Return type
    
    if (!table.add_symbol(entry)) {
        throw std::runtime_error("语义错误：符号 '" + entry.name + "' 重复定义。");
    }
    
    match(TokenCategory::OPERATOR); // ';'
    block();
}

/**
 * @brief 解析 <ParameterList>
 */
std::unique_ptr<SubprogramInfo> Parser::parameter_list() {
    auto info = std::make_unique<SubprogramInfo>();
    match(TokenCategory::OPERATOR); // '('
    
    if (current_token.category != TokenCategory::IDENTIFIER) { // Empty parameter list
        match(TokenCategory::OPERATOR); // ')'
        return info;
    }

    parameter(*info);
    while (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index-1] == ";") {
        advance();
        parameter(*info);
    }
    
    match(TokenCategory::OPERATOR); // ')'
    return info;
}

/**
 * @brief 解析 <Parameter>
 */
void Parser::parameter(SubprogramInfo& info) {
    std::vector<Token> id_list = identifier_list();
    match(TokenCategory::OPERATOR); // ':'
    TypeEntry* param_type = type();

    for (const auto& id_token : id_list) {
        SymbolEntry entry;
        entry.name = table.get_simple_identifier_table()[id_token.index - 1];
        entry.category = SymbolCategory::PARAMETER;
        entry.type = param_type;
        entry.address = current_address;
        entry.scope_level = table.get_current_scope_level() + 1; // Parameters are in the next scope
        if (!table.add_symbol(entry)) {
            throw std::runtime_error("语义错误：参数 '" + entry.name + "' 重复定义。");
        }
        info.parameters.push_back(table.find_symbol(entry.name, true));
        current_address += param_type->size;
    }
}

/**
 * @brief 解析 <Type>
 */
TypeEntry* Parser::type() {
    TypeEntry* var_type = new TypeEntry();
    if (current_token.category != TokenCategory::KEYWORD) {
        throw std::runtime_error("语法错误：应为类型关键字。");
    }
    std::string type_name = table.get_keyword_table()[current_token.index - 1];
    if (type_name == "integer") {
        var_type->kind = TypeKind::SIMPLE;
        var_type->size = 4;
    } else if (type_name == "real") {
        var_type->kind = TypeKind::SIMPLE;
        var_type->size = 8;
    } else {
        delete var_type;
        throw std::runtime_error("不支持的变量类型： " + type_name);
    }
    table.add_type(var_type);
    match(TokenCategory::KEYWORD);
    return var_type;
}

/**
 * @brief 解析 <IdentifierList> ::= <Identifier> { , <Identifier> }
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
 */
void Parser::compound_statement() {
    if (table.get_keyword_table()[current_token.index - 1] != "begin") throw std::runtime_error("语法错误：应为 'begin'。");
    match(TokenCategory::KEYWORD); // 'begin'
    statement_list();
    if (table.get_keyword_table()[current_token.index - 1] != "end") throw std::runtime_error("语法错误：应为 'end'。");
    match(TokenCategory::KEYWORD); // 'end'
}

/**
 * @brief 解析 <StatementList> ::= <Statement> { ; <Statement> }
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
 * @brief 解析 <Statement>
 */
void Parser::statement() {
    if (current_token.category == TokenCategory::IDENTIFIER) {
        SymbolEntry* symbol = table.find_symbol(table.get_simple_identifier_table()[current_token.index - 1]);
        if (!symbol) throw std::runtime_error("语义错误：未声明的标识符。");
        
        // Look ahead to see if it's a subprogram call or assignment
        Lexer temp_lexer = lexer;
        Token lookahead_token = temp_lexer.get_next_token();

        if (lookahead_token.category == TokenCategory::OPERATOR && table.get_operator_table()[lookahead_token.index-1] == "(") {
            subprogram_call(symbol);
        } else {
            assignment_statement();
        }
    } else if (current_token.category == TokenCategory::KEYWORD) {
        const auto& keyword = table.get_keyword_table()[current_token.index - 1];
        if (keyword == "if") if_statement();
        else if (keyword == "while") while_statement();
        else if (keyword == "begin") compound_statement();
        else if (keyword == "print") print_statement();
    }
    // ε (empty statement) is handled by advancing past a semicolon
}

/**
 * @brief 解析 <AssignmentStatement> ::= <Identifier> := <Expression>
 */
void Parser::assignment_statement() {
    const auto& id_name = table.get_simple_identifier_table()[current_token.index - 1];
    SymbolEntry* left_entry = table.find_symbol(id_name);
    if (!left_entry) throw std::runtime_error("语义错误：未声明的标识符 '" + id_name + "'。");
    
    match(TokenCategory::IDENTIFIER);
    if (table.get_operator_table()[current_token.index - 1] != ":=") throw std::runtime_error("语法错误：应为 ':='。");
    match(TokenCategory::OPERATOR); // ':='
    
    Operand right = expression();
    
    // Check for function return value assignment
    if (left_entry->category == SymbolCategory::FUNCTION && left_entry->scope_level == table.get_current_scope_level() -1) {
         emit(OpCode::RETURN, right, {}, {});
         return;
    }

    Operand left = {Operand::Type::IDENTIFIER, left_entry->address, left_entry->name};

    // 移除在Parser中进行的即时优化（常量折叠和拷贝传播）。
    // 优化工作将完全交由Optimizer模块处理。
    // 按原样生成赋值指令，将表达式的结果赋给左侧变量。
    emit(OpCode::ASSIGN, right, {}, left);
}

/**
 * @brief 解析 <SubprogramCall>
 */
Operand Parser::subprogram_call(SymbolEntry* symbol) {
    if (symbol->category != SymbolCategory::FUNCTION && symbol->category != SymbolCategory::PROCEDURE) {
        throw std::runtime_error("语义错误：'" + symbol->name + "' 不是函数或过程。");
    }

    match(TokenCategory::IDENTIFIER); // Consume subprogram name
    match(TokenCategory::OPERATOR); // '('
    
    std::vector<Operand> args;
    if (current_token.category != TokenCategory::OPERATOR || table.get_operator_table()[current_token.index-1] != ")") {
        args.push_back(expression());
        while (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index-1] == ",") {
            advance();
            args.push_back(expression());
        }
    }
    
    match(TokenCategory::OPERATOR); // ')'

    // Semantic check for argument count
    if (args.size() != symbol->subprogram_info->parameters.size()) {
        throw std::runtime_error("语义错误：'" + symbol->name + "' 的参数数量不正确。");
    }

    for (const auto& arg : args) {
        emit(OpCode::PARAM, arg, {}, {});
    }

    Operand callee = {Operand::Type::IDENTIFIER, symbol->address, symbol->name};
    Operand num_params = {Operand::Type::CONSTANT, (int)args.size(), std::to_string(args.size())};

    if (symbol->category == SymbolCategory::FUNCTION) {
        Operand result_temp = new_temp();
        emit(OpCode::CALL, callee, num_params, result_temp);
        return result_temp;
    } else {
        emit(OpCode::CALL, callee, num_params, {});
        return {}; // Procedures don't return a value
    }
}

/**
 * @brief 解析 <IfStatement> ::= if <Condition> then <Statement> [else <Statement>]
 */
void Parser::if_statement() {
    match(TokenCategory::KEYWORD); // 'if'
    Operand cond = condition();
    if (table.get_keyword_table()[current_token.index - 1] != "then") throw std::runtime_error("语法错误：应为 'then'。");
    match(TokenCategory::KEYWORD); // 'then'
    
    int false_jump_quad = quadruples.size();
    emit(OpCode::JPF, cond, {}, {}); // Placeholder for jump target

    statement();

    if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index - 1] == "else") {
        advance();
        int skip_else_quad = quadruples.size();
        emit(OpCode::JMP, {}, {}, {}); // Jump over the else part
        backpatch(false_jump_quad, quadruples.size());
        statement();
        backpatch(skip_else_quad, quadruples.size());
    } else {
        backpatch(false_jump_quad, quadruples.size());
    }
}

/**
 * @brief 解析 <WhileStatement> ::= while <Condition> do <Statement>
 */
void Parser::while_statement() {
    match(TokenCategory::KEYWORD); // 'while'
    int loop_start = quadruples.size();
    Operand cond = condition();
    if (table.get_keyword_table()[current_token.index - 1] != "do") throw std::runtime_error("语法错误：应为 'do'。");
    match(TokenCategory::KEYWORD); // 'do'

    int false_jump_quad = quadruples.size();
    emit(OpCode::JPF, cond, {}, {});

    statement();
    
    emit(OpCode::JMP, {}, {}, {Operand::Type::LABEL, loop_start, "L" + std::to_string(loop_start)});
    backpatch(false_jump_quad, quadruples.size());
}

/**
 * @brief 解析 <PrintStatement> ::= print ( <Expression> )
 */
void Parser::print_statement() {
    match(TokenCategory::KEYWORD); // 消耗 'print' 关键字

    if (table.get_operator_table()[current_token.index - 1] != "(") {
        throw std::runtime_error("语法错误：应为 '('。");
    }
    match(TokenCategory::OPERATOR); // 消耗 '('

    // 解析括号内的表达式，它的结果就是要打印的东西
    Operand expr_to_print = expression();

    if (table.get_operator_table()[current_token.index - 1] != ")") {
        throw std::runtime_error("语法错误：应为 ')'。");
    }
    match(TokenCategory::OPERATOR); // 消耗 ')'

    // 生成一条新的 PRINT 四元式
    emit(OpCode::PRINT, expr_to_print, {}, {});
}

/**
 * @brief 解析 <Condition> ::= <Expression> <RelationalOp> <Expression>
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
 * @brief 解析 <Expression> ::= <Term> { (+|-) <Term> }
 */
Operand Parser::expression() {
    Operand result = term();
    while (current_token.category == TokenCategory::OPERATOR) {
        const auto& op_str = table.get_operator_table()[current_token.index - 1];
        if (op_str == "+" || op_str == "-") {
            OpCode op = (op_str == "+") ? OpCode::ADD : OpCode::SUB;
            advance();
            Operand term_val = term();
            Operand new_result = new_temp();
            emit(op, result, term_val, new_result);
            result = new_result;
        } else {
            break;
        }
    }
    return result;
}

/**
 * @brief 解析 <Term> ::= <Factor> { (*|/) <Factor> }
 */
Operand Parser::term() {
    Operand result = factor();
    while (current_token.category == TokenCategory::OPERATOR) {
        const auto& op_str = table.get_operator_table()[current_token.index - 1];
        if (op_str == "*" || op_str == "/") {
            OpCode op = (op_str == "*") ? OpCode::MUL : OpCode::DIV;
            advance();
            Operand factor_val = factor();
            Operand new_result = new_temp();
            emit(op, result, factor_val, new_result);
            result = new_result;
        } else {
            break;
        }
    }
    return result;
}

/**
 * @brief 解析 <Factor> ::= <Identifier> | <Constant> | ( <Expression> ) | <SubprogramCall>
 */
Operand Parser::factor() {
    if (current_token.category == TokenCategory::IDENTIFIER) {
        SymbolEntry* symbol = table.find_symbol(table.get_simple_identifier_table()[current_token.index - 1]);
        if (!symbol) throw std::runtime_error("语义错误：未声明的标识符。");

        // Look ahead to see if it's a function call
        Lexer temp_lexer = lexer;
        Token lookahead_token = temp_lexer.get_next_token();

        if (lookahead_token.category == TokenCategory::OPERATOR && table.get_operator_table()[lookahead_token.index-1] == "(") {
            if (symbol->category != SymbolCategory::FUNCTION) {
                 throw std::runtime_error("语义错误：'" + symbol->name + "' 不是函数，不能在表达式中调用。");
            }
            return subprogram_call(symbol);
        } else {
            advance();
            return {Operand::Type::IDENTIFIER, symbol->address, symbol->name};
        }
    } else if (current_token.category == TokenCategory::CONSTANT) {
        int const_index = current_token.index - 1;
        double const_val = table.get_constant_table()[const_index];
        advance();
        return {Operand::Type::CONSTANT, const_index, std::to_string(const_val)};
    } else if (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index - 1] == "(") {
        advance();
        Operand result = expression();
        match(TokenCategory::OPERATOR); // ')'
        return result;
    } else {
        throw std::runtime_error("语法错误：无效的因子。");
    }
}

/**
 * @brief 解析 <RelationalOp> ::= = | <> | < | <= | > | >=
 */
OpCode Parser::relational_op() {
    const auto& op = table.get_operator_table()[current_token.index - 1];
    advance();
    if (op == "=") return OpCode::EQ;
    if (op == "<>") return OpCode::NE;
    if (op == "<") return OpCode::LT;
    if (op == "<=") return OpCode::LE;
    if (op == ">") return OpCode::GT;
    if (op == ">=") return OpCode::GE;
    throw std::runtime_error("语法错误：应为关系运算符。");
}
