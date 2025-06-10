#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <vector>
#include <string>
#include "../parser/Quadruple.h"
#include "../semantic_analyzer/SymbolTable.h"

class Optimizer {
public:
    static std::vector<Quadruple> optimize(const std::vector<Quadruple>& quads, SymbolTable& symbol_table);

private:
    // 已有的优化函数
    static bool constant_folding(std::vector<Quadruple>& quads, SymbolTable& symbol_table);
    static bool eliminate_common_subexpressions(std::vector<Quadruple>& quads);
    static bool dead_code_elimination(std::vector<Quadruple>& quads, const SymbolTable& symbol_table);
    static bool copy_propagation(std::vector<Quadruple>& quads);

    // ===================================================
    // 新增的循环优化函数声明
    // ===================================================
    static bool loop_invariant_code_motion(std::vector<Quadruple>& quads);
    static bool strength_reduction(std::vector<Quadruple>& quads, SymbolTable& symbol_table);
};

#endif // OPTIMIZER_H