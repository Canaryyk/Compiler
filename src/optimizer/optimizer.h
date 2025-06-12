#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include "../parser/Quadruple.h"
#include "../semantic_analyzer/SymbolTable.h"

// 基本块结构
struct BasicBlock {
    std::vector<Quadruple> quads;  // 基本块中的四元式
    std::set<int> predecessors;    // 前驱基本块
    std::set<int> successors;      // 后继基本块
    std::set<std::string> def;     // 在块中定义的变量
    std::set<std::string> use;     // 在块中使用的变量
    std::set<std::string> live_in; // 块入口处的活跃变量
    std::set<std::string> live_out;// 块出口处的活跃变量
};

class Optimizer {
public:
    // 唯一的公共接口，用于启动整个优化过程
    static std::vector<Quadruple> optimize(const std::vector<Quadruple>& quads, SymbolTable& symbol_table);

private:
    // 所有具体的优化过程都作为私有静态辅助函数
    static bool constant_folding(std::vector<Quadruple>& quads, SymbolTable& symbol_table);
    static bool copy_propagation(std::vector<Quadruple>& quads);
    static bool eliminate_common_subexpressions(std::vector<Quadruple>& quads);
    static bool dead_code_elimination(std::vector<Quadruple>& quads, const SymbolTable& symbol_table);

    // 基本块相关的新函数
    static std::vector<BasicBlock> build_basic_blocks(const std::vector<Quadruple>& quads);
    static void compute_def_use_sets(std::vector<BasicBlock>& blocks);
    static void compute_live_variables(std::vector<BasicBlock>& blocks);
    static std::vector<Quadruple> optimize_basic_blocks(std::vector<BasicBlock>& blocks, SymbolTable& symbol_table);
    
    // 辅助函数
    static std::string get_operand_name(const Operand& op, const SymbolTable& symbol_table);
    static bool is_jump_op(OpCode op);
    static bool is_label_op(OpCode op);
     static void recompute_jump_targets(std::vector<Quadruple>& quads);
};

#endif // OPTIMIZER_H