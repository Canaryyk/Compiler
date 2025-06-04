#include "SymbolTable.h"

SymbolTable::SymbolTable() : current_scope_level_(-1) { // Initialize with no active scope
    enter_scope(); // Enter global scope (level 0)
}

// 在当前作用域插入符号
bool SymbolTable::insert(const std::string& name, SymbolType symbol_type, TokenType data_type) {
    if (scoped_tables_.empty() || current_scope_level_ < 0) {
        // Should not happen if constructor called enter_scope()
        return false; 
    }
    // 检查当前作用域是否已存在该符号
    auto& current_scope_map = scoped_tables_[current_scope_level_];
    if (current_scope_map.find(name) != current_scope_map.end()) {
        return false; // Symbol already exists in the current scope
    }
    current_scope_map[name] = std::make_shared<SymbolEntry>(name, symbol_type, data_type, current_scope_level_);
    return true;
}

// 从内向外查找符号
std::shared_ptr<SymbolEntry> SymbolTable::lookup(const std::string& name) const {
    return find_in_scopes(name);
}

void SymbolTable::enter_scope() {
    current_scope_level_++;
    scoped_tables_.emplace_back(); // Add a new map for the new scope
}

void SymbolTable::exit_scope() {
    if (current_scope_level_ >= 0) {
        scoped_tables_.pop_back(); // Remove the map for the exited scope
        current_scope_level_--;
    }
    // else: Error, trying to exit a scope that doesn't exist (e.g., global scope too early)
}

int SymbolTable::get_current_scope_level() const {
    return current_scope_level_;
}

std::shared_ptr<SymbolEntry> SymbolTable::find_in_scopes(const std::string& name) const {
    for (int i = current_scope_level_; i >= 0; --i) {
        if (i < static_cast<int>(scoped_tables_.size())) { // Ensure index is valid
            const auto& scope_map = scoped_tables_[i];
            auto it = scope_map.find(name);
            if (it != scope_map.end()) {
                return it->second; // Found
            }
        }
    }
    return nullptr; // Not found in any scope
} 
