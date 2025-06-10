#include "Optimizer.h"
#include <unordered_map>
#include <set>
#include <string>
#include <algorithm>
#include <vector>

// ... 您已有的 Expression 结构体和哈希函数 ...
struct Expression {
    OpCode op;
    std::string arg1_name;
    std::string arg2_name;

    bool operator==(const Expression& other) const {
        return op == other.op && arg1_name == other.arg1_name && arg2_name == other.arg2_name;
    }
};
namespace std {
    template <> struct hash<Expression> {
        size_t operator()(const Expression& e) const {
            return hash<int>()(static_cast<int>(e.op)) ^ (hash<string>()(e.arg1_name) << 1) ^ hash<string>()(e.arg2_name);
        }
    };
}


std::vector<Quadruple> Optimizer::optimize(const std::vector<Quadruple>& input_quads, SymbolTable& symbol_table) {
    std::vector<Quadruple> quads = input_quads;
    bool changed = true;

    while (changed) {
        changed = false;
        changed |= constant_folding(quads, symbol_table);
        changed |= copy_propagation(quads);
        changed |= eliminate_common_subexpressions(quads);
        // 新增：在常规优化之后，进行循环优化
        changed |= loop_invariant_code_motion(quads);
        changed |= strength_reduction(quads, symbol_table);
        changed |= dead_code_elimination(quads, symbol_table); 
    }

    return quads;
}


// ===================================================================
// 新增的循环优化函数
// ===================================================================

/**
 * @brief 不变表达式外提 (Loop-Invariant Code Motion)
 * @details 将循环中结果不变的计算提到循环外部。
 */
bool Optimizer::loop_invariant_code_motion(std::vector<Quadruple>& quads) {
    // 步骤 1: 识别循环 (使用向后跳转的简化方法)
    std::vector<std::pair<size_t, size_t>> loops;
    for (size_t i = 0; i < quads.size(); ++i) {
        const auto& q = quads[i];
        if (q.op == OpCode::JUMP && q.result.type == Operand::Type::LABEL && q.result.index >= 0 && static_cast<size_t>(q.result.index) < i) {
            loops.push_back({static_cast<size_t>(q.result.index), i});
        }
    }

    bool changed_overall = false;
    for (const auto& loop_range : loops) {
        size_t loop_start = loop_range.first;
        size_t loop_end = loop_range.second;
        
        // 步骤 2: 找出在循环内部被赋值的所有变量
        std::set<std::string> defined_in_loop;
        for (size_t i = loop_start; i <= loop_end; ++i) {
            if (quads[i].result.type == Operand::Type::IDENTIFIER || quads[i].result.type == Operand::Type::TEMPORARY) {
                defined_in_loop.insert(quads[i].result.name);
            }
        }
        
        std::vector<Quadruple> invariant_code;
        for (size_t i = loop_start; i <= loop_end; ) {
            auto& q = quads[i];
            bool is_invariant = false;

            // 只处理可以外提的算术运算
            if (q.op == OpCode::ADD || q.op == OpCode::SUB || q.op == OpCode::MUL || q.op == OpCode::DIV) {
                bool arg1_is_invariant = (q.arg1.type == Operand::Type::CONSTANT || defined_in_loop.find(q.arg1.name) == defined_in_loop.end());
                bool arg2_is_invariant = (q.arg2.type == Operand::Type::CONSTANT || defined_in_loop.find(q.arg2.name) == defined_in_loop.end());

                if (arg1_is_invariant && arg2_is_invariant) {
                    is_invariant = true;
                }
            }
            
            if (is_invariant) {
                changed_overall = true;
                invariant_code.push_back(q);
                // 从循环中删除该指令
                quads.erase(quads.begin() + i);
                loop_end--; // 调整循环边界
                // 不增加 i，因为当前位置已经被新指令取代
            } else {
                i++;
            }
        }

        // 步骤 3: 将不变量代码插入到循环开始之前
        if (!invariant_code.empty()) {
            quads.insert(quads.begin() + loop_start, invariant_code.begin(), invariant_code.end());
            return true; // 一次只处理一个循环，让主循环重新迭代
        }
    }

    return changed_overall;
}

/**
 * @brief 消减运算强度 (Strength Reduction)
 * @details 将循环内的高强度乘法运算替换为低强度的加法运算。
 */
bool Optimizer::strength_reduction(std::vector<Quadruple>& quads, SymbolTable& symbol_table) {
    // 此函数需要一个创建新临时变量的机制，我们假设符号表可以辅助
    // 为了简化，我们假设临时变量可以简单地通过命名来创建
    static int temp_counter_for_strength_reduction = 0;

    std::vector<std::pair<size_t, size_t>> loops;
    for (size_t i = 0; i < quads.size(); ++i) {
        const auto& q = quads[i];
        if (q.op == OpCode::JUMP && q.result.type == Operand::Type::LABEL && q.result.index >= 0 && static_cast<size_t>(q.result.index) < i) {
            loops.push_back({static_cast<size_t>(q.result.index), i});
        }
    }
    
    for (const auto& loop_range : loops) {
        size_t loop_start = loop_range.first;
        size_t loop_end = loop_range.second;

        std::string induction_var_name = "";
        Operand induction_step;

        // 1. 寻找基本归纳变量 (i = i + C)
        for (size_t i = loop_start; i <= loop_end; ++i) {
            auto& q = quads[i];
            if (q.op == OpCode::ADD && q.result.name == q.arg1.name && q.arg2.type == Operand::Type::CONSTANT) {
                induction_var_name = q.result.name;
                induction_step = q.arg2;
                break;
            }
        }
        
        if (induction_var_name.empty()) continue; // 没有找到归纳变量

        // 2. 寻找可以进行强度削减的乘法指令 (t = i * C)
        for (size_t i = loop_start; i <= loop_end; ++i) {
            auto& q = quads[i];
            if (q.op == OpCode::MUL) {
                std::string var_operand_name;
                Operand const_operand;

                if (q.arg1.name == induction_var_name && q.arg2.type == Operand::Type::CONSTANT) {
                    var_operand_name = q.arg1.name;
                    const_operand = q.arg2;
                } else if (q.arg2.name == induction_var_name && q.arg1.type == Operand::Type::CONSTANT) {
                    var_operand_name = q.arg2.name;
                    const_operand = q.arg1;
                } else {
                    continue; // 不是归纳变量与常量的乘法
                }

                // 3. 执行替换
                std::string new_temp_name = "s" + std::to_string(temp_counter_for_strength_reduction++);
                Operand new_temp_op = {Operand::Type::TEMPORARY, -1, new_temp_name};

                // a. 在循环前插入初始化语句: s = i * C
                Quadruple init_quad = {OpCode::MUL, q.arg1, q.arg2, new_temp_op};
                quads.insert(quads.begin() + loop_start, init_quad);

                // b. 在循环内用 t := s 替换 t := i * C
                q.op = OpCode::ASSIGN;
                q.arg1 = new_temp_op;
                q.arg2 = {};

                // c. 在归纳变量更新后，插入 s := s + (C * step)
                double const_val = symbol_table.get_constant_table()[const_operand.index];
                double step_val = symbol_table.get_constant_table()[induction_step.index];
                int new_const_index = symbol_table.lookup_or_add_constant(const_val * step_val);
                Operand new_const_op = {Operand::Type::CONSTANT, new_const_index, std::to_string(const_val*step_val)};
                
                Quadruple update_quad = {OpCode::ADD, new_temp_op, new_const_op, new_temp_op};
                // 找到归纳变量更新的位置
                size_t induction_update_pos = 0;
                for(size_t j=loop_start; j<=loop_end+1; ++j) {
                    if(quads[j].result.name == induction_var_name) {
                        induction_update_pos = j;
                        break;
                    }
                }
                quads.insert(quads.begin() + induction_update_pos + 1, update_quad);
                
                return true; // 一次只处理一个，让主循环重新开始
            }
        }
    }
    return false;
}


// ===================================================================
// 您已有的其他优化函数
// ===================================================================
bool Optimizer::constant_folding(std::vector<Quadruple>& quads, SymbolTable& symbol_table) {
    bool changed = false;
    for (auto& q : quads) {
        if ((q.op == OpCode::ADD || q.op == OpCode::SUB || q.op == OpCode::MUL || q.op == OpCode::DIV) &&
            q.arg1.type == Operand::Type::CONSTANT && q.arg2.type == Operand::Type::CONSTANT) {
            if (q.arg1.index < 0 || static_cast<size_t>(q.arg1.index) >= symbol_table.get_constant_table().size() ||
                q.arg2.index < 0 || static_cast<size_t>(q.arg2.index) >= symbol_table.get_constant_table().size()) {
                continue;
            }
            double v1 = symbol_table.get_constant_table()[q.arg1.index];
            double v2 = symbol_table.get_constant_table()[q.arg2.index];
            double result;
            switch (q.op) {
                case OpCode::ADD: result = v1 + v2; break;
                case OpCode::SUB: result = v1 - v2; break;
                case OpCode::MUL: result = v1 * v2; break;
                case OpCode::DIV: if (v2 == 0) continue; result = v1 / v2; break;
                default: continue;
            }
            int const_index = symbol_table.lookup_or_add_constant(result);
            q.op = OpCode::ASSIGN;
            q.arg1 = { Operand::Type::CONSTANT, const_index, std::to_string(result) };
            q.arg2 = {};
            changed = true;
        }
    }
    return changed;
}

bool Optimizer::eliminate_common_subexpressions(std::vector<Quadruple>& quads) {
    bool changed = false;
    std::unordered_map<Expression, std::string> available_exprs;
    std::unordered_map<std::string, std::vector<Expression>> var_to_exprs;
    for (auto& q : quads) {
        if (q.result.type == Operand::Type::IDENTIFIER || q.result.type == Operand::Type::TEMPORARY) {
            if (var_to_exprs.count(q.result.name)) {
                for (const auto& expr_to_invalidate : var_to_exprs[q.result.name]) {
                    available_exprs.erase(expr_to_invalidate);
                }
                var_to_exprs.erase(q.result.name);
            }
        }
        if (q.op == OpCode::ADD || q.op == OpCode::SUB || q.op == OpCode::MUL || q.op == OpCode::DIV) {
            Expression expr = {q.op, q.arg1.name, q.arg2.name};
            if ((q.op == OpCode::ADD || q.op == OpCode::MUL) && q.arg1.name > q.arg2.name) {
                std::swap(expr.arg1_name, expr.arg2_name);
            }
            if (available_exprs.count(expr)) {
                std::string temp_name = available_exprs.at(expr);
                q.op = OpCode::ASSIGN;
                q.arg1 = {Operand::Type::TEMPORARY, -1, temp_name};
                q.arg2 = {};
                changed = true;
            } else {
                available_exprs[expr] = q.result.name;
                var_to_exprs[q.arg1.name].push_back(expr);
                var_to_exprs[q.arg2.name].push_back(expr);
            }
        }
    }
    return changed;
}

bool Optimizer::copy_propagation(std::vector<Quadruple>& quads) {
    bool changed = false;
    std::unordered_map<std::string, std::string> copies;
    for (auto& q : quads) {
        if (q.arg1.type == Operand::Type::IDENTIFIER || q.arg1.type == Operand::Type::TEMPORARY) {
            if (copies.count(q.arg1.name)) {
                q.arg1.name = copies.at(q.arg1.name);
                changed = true;
            }
        }
        if (q.arg2.type == Operand::Type::IDENTIFIER || q.arg2.type == Operand::Type::TEMPORARY) {
            if (copies.count(q.arg2.name)) {
                q.arg2.name = copies.at(q.arg2.name);
                changed = true;
            }
        }
        if (q.result.type == Operand::Type::IDENTIFIER || q.result.type == Operand::Type::TEMPORARY) {
            const std::string& var_name = q.result.name;
            copies.erase(var_name);
            for (auto it = copies.begin(); it != copies.end(); ) {
                if (it->second == var_name) {
                    it = copies.erase(it);
                } else {
                    ++it;
                }
            }
        }
        if (q.op == OpCode::ASSIGN && (q.arg1.type == Operand::Type::IDENTIFIER || q.arg1.type == Operand::Type::TEMPORARY)) {
            if (q.result.name != q.arg1.name) {
                copies[q.result.name] = q.arg1.name;
            }
        }
    }
    return changed;
}

bool Optimizer::dead_code_elimination(std::vector<Quadruple>& quads, const SymbolTable& symbol_table) {
    std::set<std::string> live_vars;
    for (const auto& entry : symbol_table.get_symbol_entries()) {
        live_vars.insert(entry.name);
    }
    size_t original_size = quads.size();
    std::vector<Quadruple> new_quads;
    new_quads.reserve(original_size);
    for (auto it = quads.rbegin(); it != quads.rend(); ++it) {
        const auto& q = *it;
        bool is_live = false;
        if (q.op == OpCode::JUMP || q.op == OpCode::JUMP_IF_FALSE || q.op == OpCode::CALL || q.op == OpCode::PRINT) {
            is_live = true;
        } else if (q.result.type == Operand::Type::IDENTIFIER || q.result.type == Operand::Type::TEMPORARY) {
            if (live_vars.count(q.result.name)) {
                is_live = true;
            }
        }
        if (is_live) {
            new_quads.push_back(q);
            if (q.result.type == Operand::Type::IDENTIFIER || q.result.type == Operand::Type::TEMPORARY) {
                live_vars.erase(q.result.name);
            }
            if (q.arg1.type == Operand::Type::IDENTIFIER || q.arg1.type == Operand::Type::TEMPORARY) {
                live_vars.insert(q.arg1.name);
            }
            if (q.arg2.type == Operand::Type::IDENTIFIER || q.arg2.type == Operand::Type::TEMPORARY) {
                live_vars.insert(q.arg2.name);
            }
        }
    }
    std::reverse(new_quads.begin(), new_quads.end());
    if (new_quads.size() != original_size) {
        quads = new_quads;
        return true;
    }
    return false;
}