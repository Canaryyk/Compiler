#include "SymbolTable.h"

SymbolTable::SymbolTable() {
    initialize_keywords();
    initialize_operators();
    // 进入全局作用域
    enter_scope();
}

void SymbolTable::initialize_keywords() {
    keyword_table = {"program", "var", "begin", "end", "if", "then", "else", "while", "do", "integer", "real", "char"};
    for (size_t i = 0; i < keyword_table.size(); ++i) {
        keyword_map[keyword_table[i]] = i;
    }
}

void SymbolTable::initialize_operators() {
    operator_table = {".", ":", ";", ",", ":=", "=", "<>", "<", "<=", ">", ">=", "+", "-", "*", "/", "(", ")"};
    for (size_t i = 0; i < operator_table.size(); ++i) {
        operator_map[operator_table[i]] = i;
    }
}

int SymbolTable::find_keyword(const std::string& name) {
    auto it = keyword_map.find(name);
    if (it != keyword_map.end()) {
        return it->second;
    }
    return -1;
}

int SymbolTable::find_operator(const std::string& op) {
    auto it = operator_map.find(op);
    if (it != operator_map.end()) {
        return it->second;
    }
    return -1;
}

void SymbolTable::enter_scope() {
    scope_stack.emplace_back();
}

void SymbolTable::exit_scope() {
    if (scope_stack.size() > 1) { // 防止退出全局作用域
        scope_stack.pop_back();
    }
}

bool SymbolTable::add_symbol(const SymbolEntry& entry) {
    if (scope_stack.empty()) {
        return false; // 不应该发生
    }

    auto& current_scope = scope_stack.back();
    if (current_scope.count(entry.name)) {
        return false; // 符号重定义
    }

    int index = symbol_entries.size();
    symbol_entries.push_back(entry);
    current_scope[entry.name] = index;
    return true;
}

SymbolEntry* SymbolTable::find_symbol(const std::string& name, bool find_in_current_scope_only) {
    if (scope_stack.empty()) {
        return nullptr;
    }

    if (find_in_current_scope_only) {
        auto& current_scope = scope_stack.back();
        auto it = current_scope.find(name);
        if (it != current_scope.end()) {
            return &symbol_entries[it->second];
        }
    } else {
        // 从当前作用域向外层作用域查找
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto& scope = *it;
            auto symbol_it = scope.find(name);
            if (symbol_it != scope.end()) {
                return &symbol_entries[symbol_it->second];
            }
        }
    }

    return nullptr;
}

int SymbolTable::lookup_or_add_constant(double value) {
    auto it = constant_map.find(value);
    if (it != constant_map.end()) {
        return it->second;
    }
    int index = constant_table.size();
    constant_table.push_back(value);
    constant_map[value] = index;
    return index;
}

int SymbolTable::add_identifier_for_lexer(const std::string& name) {
    auto it = simple_identifier_map.find(name);
    if (it != simple_identifier_map.end()) {
        return it->second;
    }
    int index = simple_identifier_table.size();
    simple_identifier_table.push_back(name);
    simple_identifier_map[name] = index;
    return index;
}

const std::vector<std::string>& SymbolTable::get_keyword_table() const {
    return keyword_table;
}

const std::vector<std::string>& SymbolTable::get_operator_table() const {
    return operator_table;
}

const std::vector<std::string>& SymbolTable::get_simple_identifier_table() const {
    return simple_identifier_table;
}

const std::vector<double>& SymbolTable::get_constant_table() const {
    return constant_table;
}

const std::vector<SymbolEntry>& SymbolTable::get_symbol_entries() const {
    return symbol_entries;
}

TypeEntry* SymbolTable::add_type(TypeEntry* type) {
    type_table.emplace_back(type);
    return type_table.back().get();
}

int SymbolTable::get_current_scope_level() const {
    if (scope_stack.empty()) {
        return 0;
    }
    return scope_stack.size() - 1;
}
