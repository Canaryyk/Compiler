#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>


enum class SymbolCategory {
    VARIABLE,   ///< 变量
    CONSTANT,   ///< 常量
    FUNCTION,   ///< 函数
    PROCEDURE,  //过程
    PARAMETER,   //形参
    TYPE_NAME   // 类型名
};

/**
 * @brief 类型的种类，用于区分基本类型、数组、记录等。
 */
enum class TypeKind {
    SIMPLE,     ///< 基本类型 (如 integer, real)
    ARRAY,      ///< 数组
    RECORD      ///< 记录 (结构体)
};

// 前向声明，因为 RecordField 和 ArrayInfo 会引用 TypeEntry
struct TypeEntry;
//前向声明
struct SymbolEntry;

/**
 * @brief 描述数组类型的信息。
 * (对应一些编译器教材中的 AINFL 结构)
 */

struct ArrayInfo {
    TypeEntry* element_type; ///< 数组元素的类型。
    int lower_bound;         ///< 数组下界。
    int upper_bound;         ///< 数组上界。
};

/**
 * @brief 描述记录（结构体）中的一个字段（成员）。
 */
struct RecordField {
    std::string name;        ///< 字段名。
    TypeEntry* type;         ///< 字段的类型。
    int offset;              ///< 字段在记录内部的地址偏移量。
};

/**
 * @brief 描述记录类型的信息。
 * (对应一些编译器教材中的 RINFL 结构)
 */
struct RecordInfo {
    std::vector<RecordField> fields; ///< 记录包含的所有字段。
};

/**
 * @brief 类型表中的一个条目，用于描述一个具体的类型。
 * (对应一些编译器教材中的 TYPEL 结构)
 */
struct TypeEntry {
    TypeKind kind; ///< 该类型的种类 (SIMPLE, ARRAY, or RECORD)。
    int size;      ///< 该类型实例占用的存储空间大小（字节）。

    /**
     * @brief 根据类型的种类，存储具体的类型信息。
     * 使用 union 来节省空间，因为一个类型只能是其中一种。
     */
    union {
        // 对于 SIMPLE 类型, size 字段已足够描述其大小。
        // 若需区分不同基本类型（如 int vs real），可在此添加枚举。
        ArrayInfo* array_info;   ///< 当 kind 为 ARRAY 时，指向数组信息。
        RecordInfo* record_info; ///< 当 kind 为 RECORD 时，指向记录信息。
    } info;

    /**
     * @brief 默认构造函数，创建一个简单的、大小为0的类型。
     */
    TypeEntry() : kind(TypeKind::SIMPLE), size(0) {
        info.array_info = nullptr;
    }
};

/**
 * @brief 描述函数或过程的额外信息。
 */
struct SubprogramInfo {
    std::vector<SymbolEntry*> parameters; ///< 形参列表，指针指向符号表中的条目。
    // 可以添加更多信息，如局部变量大小等。
};

/**
 * @brief 符号表中的一个条目，代表一个在源代码中定义的符号。
 * (对应一些编译器教材中的 SYNVBL 结构)
 */
struct SymbolEntry {
    std::string name;        ///< 符号的名称 (标识符)。
    SymbolCategory category; ///< 符号的种类 (变量, 函数等)。
    TypeEntry* type;         ///< 指向该符号类型的指针。
    int address;             ///< 符号在内存中的地址或在活动记录中的偏移。
    int scope_level;         ///< 符号所在的作用域层级。
    /**
     * @brief 当符号是函数或过程时，存储其额外信息。
     * 如果 category 不是 FUNCTION 或 PROCEDURE，则此指针为 nullptr。
     */
    std::unique_ptr<SubprogramInfo> subprogram_info;

    // --- Special Member Functions ---

    // 默认构造
    SymbolEntry() = default;

    // 自定义拷贝构造函数
    SymbolEntry(const SymbolEntry& other)
        : name(other.name),
          category(other.category),
          type(other.type),
          address(other.address),
          scope_level(other.scope_level) {
        if (other.subprogram_info) {
            subprogram_info = std::make_unique<SubprogramInfo>(*other.subprogram_info);
        }
    }

    // 默认移动构造函数
    SymbolEntry(SymbolEntry&& other) noexcept = default;

    // 自定义拷贝赋值运算符
    SymbolEntry& operator=(const SymbolEntry& other) {
        if (this == &other) {
            return *this;
        }
        name = other.name;
        category = other.category;
        type = other.type;
        address = other.address;
        scope_level = other.scope_level;
        if (other.subprogram_info) {
            subprogram_info = std::make_unique<SubprogramInfo>(*other.subprogram_info);
        } else {
            subprogram_info.reset();
        }
        return *this;
    }

    // 默认移动赋值运算符
    SymbolEntry& operator=(SymbolEntry&& other) noexcept = default;
};

/**
 * @class SymbolTable
 * @brief 编译器核心组件之一，用于管理符号和作用域。
 *
 * 该类负责处理：
 * - 作用域的进入和退出。
 * - 符号的定义和查找，支持嵌套作用域。
 * - 类型的定义和管理。
 * - 关键字、操作符等静态表的维护。
 */
class SymbolTable {
public:
    /**
     * @brief 构造一个新的符号表实例。
     * 在构造时会自动初始化关键字、操作符表，并进入全局作用域。
     */
    SymbolTable();

    // === 作用域管理 ===

    /**
     * @brief 进入一个新的作用域。
     * 通常在解析到函数、过程或块语句的开始时调用。
     */
    void enter_scope();

    /**
     * @brief 退出当前作用域。
     * 通常在解析到函数、过程或块语句的结束时调用。
     */
    void exit_scope();

    // === 符号和类型管理 ===

    /**
     * @brief 向当前作用域添加一个新的符号。
     * @param entry 要添加的符号条目。
     * @return 如果成功添加（即当前作用域内无同名符号），返回 true；否则返回 false。
     */
    bool add_symbol(const SymbolEntry& entry);

    /**
     * @brief 将一个类型定义添加到类型表中。
     * @param type 指向要添加的 TypeEntry 对象的指针。该对象的所有权将移交给符号表。
     * @return 返回由符号表管理的、稳定的 TypeEntry 指针。
     */
    TypeEntry* add_type(TypeEntry* type);

    // === 查询功能 ===
    
    /**
     * @brief 获取当前作用域的层级。
     * 全局作用域为0，每进入一层，层级加1。
     * @return 当前作用域层级。
     */
    int get_current_scope_level() const;

    /**
     * @brief 查找一个符号。
     * @param name 要查找的符号名称。
     * @param find_in_current_scope_only 如果为 true，则只在当前作用域查找；
     *                                   如果为 false（默认），则从当前作用域开始，逐级向外层作用域查找。
     * @return 如果找到，返回指向 SymbolEntry 的指针；否则返回 nullptr。
     */
    SymbolEntry* find_symbol(const std::string& name, bool find_in_current_scope_only = false);
    
    /**
     * @brief (为词法分析器保留) 添加一个标识符到简易标识符表。
     * 这个表主要用于词法分析阶段，快速地给标识符一个唯一的ID。
     * @param name 标识符名称。
     * @return 该标识符在简易表中的索引。
     */
    int add_identifier_for_lexer(const std::string& name);

    /**
     * @brief 查找或添加一个常量到常量表。
     * (此功能的设计可能需要根据具体语言特性进行调整)
     * @param value 常量的值。
     * @return 该常量在常量表中的索引。
     */
    int lookup_or_add_constant(double value);

    /**
     * @brief 查找一个字符串是否是关键字。
     * @param name 要检查的字符串。
     * @return 如果是关键字，返回其在关键字表中的索引；否则返回 -1。
     */
    int find_keyword(const std::string& name);

    /**
     * @brief 查找一个字符串是否是操作符或界符。
     * @param op 要检查的字符串。
     * @return 如果是，返回其在表中的索引；否则返回 -1。
     */
    int find_operator(const std::string& op);

    // === 获取内部表的访问器 (Getters) ===
    // 主要用于调试、打印或代码生成阶段。
    const std::vector<std::string>& get_keyword_table() const;
    const std::vector<std::string>& get_operator_table() const;
    const std::vector<std::string>& get_simple_identifier_table() const;
    const std::vector<double>& get_constant_table() const;
    const std::vector<SymbolEntry>& get_symbol_entries() const;

private:
    /**
     * @brief 初始化关键字列表和映射表。
     */
    void initialize_keywords();
    
    /**
     * @brief 初始化操作符和界符列表和映射表。
     */
    void initialize_operators();

    // --- 静态表 ---
    std::vector<std::string> keyword_table;              ///< 关键字表，按下标索引。
    std::vector<std::string> operator_table;             ///< 操作符/界符表，按下标索引。
    std::unordered_map<std::string, int> keyword_map;    ///< 关键字到其索引的快速映射。
    std::unordered_map<std::string, int> operator_map;   ///< 操作符到其索引的快速映射。

    // --- 词法分析阶段使用的表 ---
    std::vector<std::string> simple_identifier_table;    ///< 词法分析用的简单标识符表。
    std::unordered_map<std::string, int> simple_identifier_map; ///< 标识符到索引的映射。

    // --- 动态表 (语义分析核心) ---
    std::vector<SymbolEntry> symbol_entries; ///< 存储所有符号条目的主表。
    std::vector<std::unique_ptr<TypeEntry>> type_table; ///< 存储所有类型定义的类型表，使用智能指针管理内存。

    /**
     * @brief 作用域栈，每个元素是一个作用域。
     * 每个作用域是一个从符号名到其在 `symbol_entries` 表中索引的映射。
     * 栈顶是当前作用域。
     */
    std::vector<std::unordered_map<std::string, int>> scope_stack; 

    // --- 常量表 ---
    std::vector<double> constant_table;                  ///< 常量表，存储浮点数值。
    std::unordered_map<double, int> constant_map;        ///< 常量值到其索引的快速映射。
};

#endif // SYMBOL_TABLE_H