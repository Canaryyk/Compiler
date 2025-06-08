#include "Parser.h"
#include <iostream>
#include <stdexcept>

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
        // 'end' is handled by statement_list and compound_statement, so we don't need a special case for it here.
        // If it's another keyword, it's a syntax error, but that will be caught by the calling function.
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
    emit(OpCode::ASSIGN, right, {}, left);
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