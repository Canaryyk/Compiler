#include "Parser.h"
#include <iostream>
#include <stdexcept>


void print_operand_details(const std::string& label, const Operand& op) {
    std::cout << label << " -> "
              << "Type: " << static_cast<int>(op.type)
              << ", Name: '" << op.name << "'"
              << ", Index: " << op.index << std::endl;
}
Parser::Parser(Lexer& lexer, SymbolTable& table)
    : lexer(lexer), table(table), temp_counter(0), label_counter(0), current_address(0) {
    advance();
}

void Parser::parse() {
    program();
}

const std::vector<Quadruple>& Parser::get_quadruples() const {
    return quadruples;
}

void Parser::advance() {
    current_token = lexer.get_next_token();
}

void Parser::match(TokenCategory category) {
    if (current_token.category == category) {
        advance();
    } else {
        throw std::runtime_error("Syntax error: Unexpected token.");
    }
}

Operand Parser::new_temp() {
    std::string temp_name = "t" + std::to_string(temp_counter);
    int index = temp_counter;
    temp_counter++;
    return Operand{Operand::Type::TEMPORARY, index, temp_name};
}

void Parser::emit(OpCode op, const Operand& arg1, const Operand& arg2, const Operand& result) {
    quadruples.push_back({op, arg1, arg2, result});
}

void Parser::backpatch(int quad_index, int target_label) {
    quadruples[quad_index].result = {Operand::Type::LABEL, target_label, "L" + std::to_string(target_label)};
}

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

void Parser::block() {
    table.enter_scope();
    if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index - 1] == "var") {
        var_declarations();
    }
    compound_statement();
    table.exit_scope();
}

void Parser::var_declarations() {
    match(TokenCategory::KEYWORD); // var
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

void Parser::compound_statement() {
    if (table.get_keyword_table()[current_token.index - 1] != "begin") throw std::runtime_error("Syntax error: Expected 'begin'.");
    match(TokenCategory::KEYWORD);
    statement_list();
    if (table.get_keyword_table()[current_token.index - 1] != "end") throw std::runtime_error("Syntax error: Expected 'end'.");
    match(TokenCategory::KEYWORD);
}

void Parser::statement_list() {
    statement();
    while (current_token.category == TokenCategory::OPERATOR && table.get_operator_table()[current_token.index - 1] == ";") {
        advance();
        if (current_token.category == TokenCategory::KEYWORD && table.get_keyword_table()[current_token.index -1] == "end") {
            break;
        }
        statement();
    }
}

void Parser::statement() {
    if (current_token.category == TokenCategory::IDENTIFIER) {
        assignment_statement();
    } else if (current_token.category == TokenCategory::KEYWORD) {
        const auto& keyword = table.get_keyword_table()[current_token.index - 1];
        if (keyword == "if") if_statement();
        else if (keyword == "while") while_statement();
        else if (keyword == "begin") compound_statement();
        // +++ 新增对 print 语句的识别 +++
        else if (keyword == "print") print_statement();
    }
}

void Parser::assignment_statement() {
    const auto& id_name = table.get_simple_identifier_table()[current_token.index - 1];
    SymbolEntry* left_entry = table.find_symbol(id_name);
    if (!left_entry) throw std::runtime_error("Semantic error: Undeclared identifier '" + id_name + "'.");
    
    Operand left = {Operand::Type::IDENTIFIER, left_entry->address, left_entry->name};
    match(TokenCategory::IDENTIFIER);
    
    if (table.get_operator_table()[current_token.index - 1] != ":=") throw std::runtime_error("Syntax error: Expected ':='.");
    match(TokenCategory::OPERATOR);
    
    Operand right = expression();

    // ==================== 全新的、更强大的修改逻辑 ====================

    // 检查：如果 expression() 的结果是一个临时变量，并且上一条指令是对这个临时变量的赋值
    if (right.type == Operand::Type::TEMPORARY && !quadruples.empty()) {
        Quadruple& last_quad = quadruples.back();
        
        // 模式一：上一条是 t0 := const1 op const2 (比如 t0 := 0.5 + 0.5)
        if (last_quad.result.name == right.name &&
            (last_quad.op == OpCode::ADD || last_quad.op == OpCode::SUB || last_quad.op == OpCode::MUL || last_quad.op == OpCode::DIV) &&
            last_quad.arg1.type == Operand::Type::CONSTANT &&
            last_quad.arg2.type == Operand::Type::CONSTANT)
        {
            // 在这里当场进行常量折叠！
            double v1 = table.get_constant_table()[last_quad.arg1.index];
            double v2 = table.get_constant_table()[last_quad.arg2.index];
            double result_val;
            switch (last_quad.op) {
                case OpCode::ADD: result_val = v1 + v2; break;
                case OpCode::SUB: result_val = v1 - v2; break;
                case OpCode::MUL: result_val = v1 * v2; break;
                case OpCode::DIV: result_val = (v2 != 0) ? v1 / v2 : 0; break;
                default: result_val = 0; // Should not happen
            }
            
            // 然后，直接修改上一条指令，把它变成一条干净的赋值指令 i := 1.0
            int const_index = table.lookup_or_add_constant(result_val);
            last_quad.op = OpCode::ASSIGN;
            last_quad.arg1 = { Operand::Type::CONSTANT, const_index, std::to_string(result_val) };
            last_quad.arg2 = {};
            last_quad.result = left; // **直接赋值给 i (left)**

        } 
        // 模式二：上一条是 t0 := some_variable (未来可能出现)
        else if (last_quad.result.name == right.name &&
                 last_quad.op == OpCode::ASSIGN)
        {
            // 直接修改上一条指令，让它直接赋值给 i
            last_quad.result = left;
        }
        else {
            // 如果不匹配任何已知优化模式，则按原样生成赋值指令
            emit(OpCode::ASSIGN, right, {}, left);
        }
    } else {
        // 如果 expression() 返回的不是临时变量，也按原样生成
        emit(OpCode::ASSIGN, right, {}, left);
    }
    // ===================== 修改结束 ====================
}
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

void Parser::print_statement() {
    match(TokenCategory::KEYWORD); // 消耗 'print' 关键字

    // 假设您的 match 函数可以检查操作符的字符串值
    // 如果不能，您需要先匹配 OPERATOR，再检查其内容是否为 "("
    if (table.get_operator_table()[current_token.index - 1] != "(") {
        throw std::runtime_error("Syntax error: Expected '('.");
    }
    match(TokenCategory::OPERATOR); // 消耗 '('

    // 解析括号内的表达式，它的结果就是要打印的东西
    Operand expr_to_print = expression();

    if (table.get_operator_table()[current_token.index - 1] != ")") {
        throw std::runtime_error("Syntax error: Expected ')'.");
    }
    match(TokenCategory::OPERATOR); // 消耗 ')'

    // 生成一条新的 PRINT 四元式
    emit(OpCode::PRINT, expr_to_print, {}, {});
}

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

Operand Parser::condition() {
    Operand left = expression();
    OpCode op = relational_op();
    Operand right = expression();
    Operand result = new_temp();
    emit(op, left, right, result);
    return result;
}

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
        case OpCode::JUMP: return "j";
        case OpCode::JUMP_IF_FALSE: return "j<";
        case OpCode::PARAM: return "param";
        case OpCode::CALL: return "call";
        case OpCode::RET: return "ret";
        case OpCode::NO_OP: return "noop";
        case OpCode::PRINT: return "PRINT";
        case OpCode::NONE: return "none";
        case OpCode::LABEL: return "label";
        default: return "op?";
    }
} 
