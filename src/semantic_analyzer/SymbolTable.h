#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "../lexer/Token.h" // For TokenType, potentially data types

// 可以定义符号类型，例如变量、函数、类型等
enum class SymbolType {
    VARIABLE,
    TYPE_DEF, // For integer, real, char types themselves if stored
    // ... other types like PROCEDURE, FUNCTION if language expands
};

// 符号表条目
struct SymbolEntry {
    std::string name;
    SymbolType symbol_type;
    TokenType data_type; // 例如 TokenType::INTEGER, TokenType::REAL
    int scope_level;     // 作用域级别
    // 可以添加其他信息，如内存地址、数组维度等

    SymbolEntry(std::string n, SymbolType st, TokenType dt, int sl)
        : name(std::move(n)), symbol_type(st), data_type(dt), scope_level(sl) {}
};

// 符号表类
class SymbolTable {
public:
    SymbolTable();

    bool insert(const std::string& name, SymbolType symbol_type, TokenType data_type);
    std::shared_ptr<SymbolEntry> lookup(const std::string& name) const;

    void enter_scope();
    void exit_scope();
    int get_current_scope_level() const;

private:
    // 使用 vector of maps 来管理作用域
    // 每个 map 代表一个作用域层级的符号表
    std::vector<std::unordered_map<std::string, std::shared_ptr<SymbolEntry>>> scoped_tables_;
    int current_scope_level_;
    
    // 辅助函数，查找当前作用域及外层作用域
    std::shared_ptr<SymbolEntry> find_in_scopes(const std::string& name) const;
};

#endif //SYMBOL_TABLE_H 