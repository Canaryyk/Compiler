/**
 * @file SymbolTable.cpp
 * @brief 实现了符号表的功能。
 *
 * 该文件包含 SymbolTable 类中声明的所有方法的具体实现，
 * 包括作用域管理、符号的添加与查找、以及各种辅助表的初始化和维护。
 */
#include "SymbolTable.h"

/**
 * @brief SymbolTable 类的构造函数。
 * 负责初始化关键字表、操作符表，并自动进入顶层（全局）作用域。
 */
SymbolTable::SymbolTable() {
    initialize_keywords();
    initialize_operators();
    // 程序开始时，进入全局作用域（第0层）
    enter_scope();
}

/**
 * @brief 初始化关键字表和快速查找的哈希表。
 * 将预定义的关键字列表加载到 vector 和 unordered_map 中。
 */
void SymbolTable::initialize_keywords() {
    keyword_table = {"program", "var", "begin", "end", "if", "then", "else", "while", "do", "integer", "real", "char", "procedure", "function"};
    for (size_t i = 0; i < keyword_table.size(); ++i) {
        keyword_map[keyword_table[i]] = i;
    }
}

/**
 * @brief 初始化操作符和界符表及相应的哈希表。
 */
void SymbolTable::initialize_operators() {
    operator_table = {".", ":", ";", ",", ":=", "=", "<>", "<", "<=", ">", ">=", "+", "-", "*", "/", "(", ")"};
    for (size_t i = 0; i < operator_table.size(); ++i) {
        operator_map[operator_table[i]] = i;
    }
}

/**
 * @brief 在哈希表中查找一个字符串是否为关键字。
 * @param name 待查字符串。
 * @return 如果是关键字，返回其ID；否则返回-1。
 */
int SymbolTable::find_keyword(const std::string& name) {
    auto it = keyword_map.find(name);
    if (it != keyword_map.end()) {
        return it->second; // 找到了，返回索引
    }
    return -1; // 未找到
}

/**
 * @brief 在哈希表中查找一个字符串是否为操作符/界符。
 * @param op 待查字符串。
 * @return 如果是，返回其ID；否则返回-1。
 */
int SymbolTable::find_operator(const std::string& op) {
    auto it = operator_map.find(op);
    if (it != operator_map.end()) {
        return it->second;
    }
    return -1;
}

/**
 * @brief 进入一个新的作用域。
 * 实现上，就是在作用域栈的顶端压入一个新的、空的哈希表。
 */
void SymbolTable::enter_scope() {
    scope_stack.emplace_back();
}

/**
 * @brief 退出当前作用域。
 * 实现上，就是弹出作用域栈顶的哈希表。
 * 为了防止意外退出全局作用域，会保留最后一个作用域。
 */
void SymbolTable::exit_scope() {
    if (scope_stack.size() > 1) { // 至少保留全局作用域
        scope_stack.pop_back();
    }
}

/**
 * @brief 在当前作用域中添加一个新符号。
 * @param entry 待添加的符号信息。
 * @return 若成功（当前作用域无重名符号）返回 true，否则返回 false。
 */
bool SymbolTable::add_symbol(const SymbolEntry& entry) {
    if (scope_stack.empty()) {
        return false; // 理论上不会发生，因为构造时就进入了全局作用域
    }

    // 获取当前作用域的哈希表
    auto& current_scope = scope_stack.back();
    // 检查是否重定义
    if (current_scope.count(entry.name)) {
        return false; // 错误：符号在该作用域内已存在
    }

    // 将符号实体添加到主符号表中，并获取其索引
    int index = symbol_entries.size();
    symbol_entries.push_back(entry);
    // 在当前作用域的哈希表中，建立名字到索引的映射
    current_scope[entry.name] = index;
    return true;
}

/**
 * @brief 查找符号。
 * @param name 待查找的符号名。
 * @param find_in_current_scope_only
 *        - true: 只在当前作用域查找，用于处理变量声明时的重名检查。
 *        - false: 从当前作用域开始，逐层向外层（全局）作用域查找，模拟嵌套作用域的可见性规则。
 * @return 若找到，返回指向该符号条目的指针；否则返回 nullptr。
 */
SymbolEntry* SymbolTable::find_symbol(const std::string& name, bool find_in_current_scope_only) {
    if (scope_stack.empty()) {
        return nullptr; // 作用域栈为空，直接返回
    }

    if (find_in_current_scope_only) {
        // 只在当前作用域（栈顶）查找
        auto& current_scope = scope_stack.back();
        auto it = current_scope.find(name);
        if (it != current_scope.end()) {
            // 如果在哈希表中找到，用其值（索引）去主符号表取回符号实体
            return &symbol_entries[it->second];
        }
    } else {
        // 从当前作用域向外层作用域依次查找
        // 使用反向迭代器 rbegin() / rend() 可以方便地从栈顶遍历到栈底
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            auto& scope = *it;
            auto symbol_it = scope.find(name);
            if (symbol_it != scope.end()) {
                // 在某个作用域找到了，立即返回结果
                return &symbol_entries[symbol_it->second];
            }
        }
    }

    return nullptr; // 所有作用域都找遍了，没找到
}

/**
 * @brief 查找或添加一个常量。
 * @param value 常量的值。
 * @return 该常量在常量表中的索引。
 */
int SymbolTable::lookup_or_add_constant(double value) {
    // 首先尝试在哈希表中查找该常量是否已存在
    auto it = constant_map.find(value);
    if (it != constant_map.end()) {
        return it->second; // 存在，直接返回索引
    }
    // 不存在，则添加到常量表和哈希表的末尾
    int index = constant_table.size();
    constant_table.push_back(value);
    constant_map[value] = index;
    return index;
}

/**
 * @brief 为词法分析器添加一个标识符到简易表中。
 * 这个函数通常在词法分析阶段被调用，为每个遇到的标识符分配一个唯一的ID。
 * 它不关心作用域，只维护一个全局的标识符列表。
 * @param name 标识符的字符串。
 * @return 该标识符在简易表中的索引。
 */
int SymbolTable::add_identifier_for_lexer(const std::string& name) {
    auto it = simple_identifier_map.find(name);
    if (it != simple_identifier_map.end()) {
        return it->second; // 已存在，返回ID
    }
    // 不存在，则添加到表末尾
    int index = simple_identifier_table.size();
    simple_identifier_table.push_back(name);
    simple_identifier_map[name] = index;
    return index;
}

// --- Getters ---
// 以下是一系列 getter 函数，用于向外部提供对内部数据表的只读访问。
// 这对于调试、打印符号表内容或在后续阶段（如代码生成）访问符号信息非常有用。

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

/**
 * @brief 将一个新的类型定义添加到类型表中。
 * 使用 unique_ptr 来管理 TypeEntry 对象的生命周期，防止内存泄漏。
 * @param type 一个指向动态分配的 TypeEntry 对象的指针。
 * @return 符号表所管理的 TypeEntry 对象的裸指针，其生命周期由符号表保证。
 */
TypeEntry* SymbolTable::add_type(TypeEntry* type) {
    type_table.emplace_back(type);
    return type_table.back().get();
}

/**
 * @brief 获取当前作用域的层级。
 * @return 返回当前作用域的深度。全局作用域为0。
 */
int SymbolTable::get_current_scope_level() const {
    if (scope_stack.empty()) {
        return 0; // 理论上不会发生
    }
    // 作用域层级 = 栈的大小 - 1 (因为层级从0开始)
    return scope_stack.size() - 1;
}
