#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// 符号的种类 (Category)
enum class SymbolCategory {
    VARIABLE,   // 变量
    CONSTANT,   // 常量
    FUNCTION,   // 函数
    PROCEDURE,  // 过程
    TYPE_NAME   // 类型名
};

// 类型的种类 (Kind)
enum class TypeKind {
    SIMPLE,     // 基本类型
    ARRAY,      // 数组
    RECORD      // 记录 (结构体)
};

// 前向声明
struct TypeEntry;

// 数组信息 (对应 AINFL)
struct ArrayInfo {
    TypeEntry* element_type; // 元素类型
    int lower_bound;         // 下界
    int upper_bound;         // 上界
};

// 记录/结构体中的一个字段
struct RecordField {
    std::string name;        // 字段名
    TypeEntry* type;         // 字段类型
    int offset;              // 在记录中的偏移量
};

// 记录信息 (对应 RINFL)
struct RecordInfo {
    std::vector<RecordField> fields;
};

// 类型表条目 (对应 TYPEL)
struct TypeEntry {
    TypeKind kind;
    int size; // 类型占用的空间大小
    union {
        // 对于 SIMPLE 类型, 可以直接使用 size，或者在这里添加一个基本类型枚举
        ArrayInfo* array_info;   // 对于 ARRAY 类型
        RecordInfo* record_info; // 对于 RECORD 类型
    } info;

    TypeEntry() : kind(TypeKind::SIMPLE), size(0) {
        info.array_info = nullptr;
    }
};

// 符号表主表条目 (对应 SYNVBL)
struct SymbolEntry {
    std::string name;
    SymbolCategory category;
    TypeEntry* type;
    int address; // 在活动记录中的地址或偏移
    int scope_level; // 所属作用域层级
};

class SymbolTable {
public:
    SymbolTable();

    // 作用域管理
    void enter_scope();
    void exit_scope();

    // 添加符号
    bool add_symbol(const SymbolEntry& entry);

    // 添加类型定义并返回指针
    TypeEntry* add_type(TypeEntry* type);

    // 获取当前作用域深度
    int get_current_scope_level() const;

    // 专为词法分析器提供的简化接口
    int add_identifier_for_lexer(const std::string& name);

    // 查找符号
    SymbolEntry* find_symbol(const std::string& name, bool find_in_current_scope_only = false);

    // 查找或添加常数，返回索引 (保留，但可能需要调整)
    int lookup_or_add_constant(double value);

    // 查找关键字，如果不是关键字则返回-1
    int find_keyword(const std::string& name);

    // 查找操作符/界符，如果不是则返回-1
    int find_operator(const std::string& op);

    // Getters for printing
    const std::vector<std::string>& get_keyword_table() const;
    const std::vector<std::string>& get_operator_table() const;
    const std::vector<std::string>& get_simple_identifier_table() const;
    const std::vector<double>& get_constant_table() const;
    const std::vector<SymbolEntry>& get_symbol_entries() const;

private:
    void initialize_keywords();
    void initialize_operators();

    // 静态表
    std::vector<std::string> keyword_table;
    std::vector<std::string> operator_table;
    std::unordered_map<std::string, int> keyword_map;
    std::unordered_map<std::string, int> operator_map;

    // 用于词法分析阶段的简单标识符表
    std::vector<std::string> simple_identifier_table;
    std::unordered_map<std::string, int> simple_identifier_map;

    // 动态表 - 新结构
    std::vector<SymbolEntry> symbol_entries; // 符号表主表
    std::vector<std::unique_ptr<TypeEntry>> type_table; // 类型表

    // 使用一个map来将名字映射到 symbol_entries 的索引，每个作用域一个map
    std::vector<std::unordered_map<std::string, int>> scope_stack; 

    // 常量表 (暂时保留)
    std::vector<double> constant_table;
    std::unordered_map<double, int> constant_map;
};

#endif // SYMBOL_TABLE_H
