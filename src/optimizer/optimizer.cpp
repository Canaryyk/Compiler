#include "Optimizer.h"
#include <unordered_map>
#include <set>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include "../parser/Parser.h"

// 辅助结构体可以保留在文件作用域
struct Expression {
    OpCode op;
    std::string arg1_name;
    std::string arg2_name;

    bool operator==(const Expression& other) const {
        return op == other.op && arg1_name == other.arg1_name && arg2_name == other.arg2_name;
    }
};

namespace std {
    template <>
    struct hash<Expression> {
        size_t operator()(const Expression& e) const {
            size_t h1 = hash<int>()(static_cast<int>(e.op));
            size_t h2 = hash<string>()(e.arg1_name);
            size_t h3 = hash<string>()(e.arg2_name);
            return h1 ^ (h2 << 1) ^ h3;
        }
    };
}

// 声明一个仅在当前文件可见的辅助函数
static void eliminate_redundant_stores_in_block(std::vector<Quadruple>& block_quads);
static void fold_temps_in_block(std::vector<Quadruple>& block_quads, const std::set<std::string>& live_out);

// ===================================================================
// Optimizer 类静态成员函数的实现
// ===================================================================

std::vector<Quadruple> Optimizer::optimize(const std::vector<Quadruple>& quads, SymbolTable& symbol_table) {
    if (quads.empty()) {
        return {};
    }

    // 步骤 0: 将基于行号的跳转转换为基于标签的跳转，使优化更安全
    std::vector<Quadruple> labeled_quads;
    labeled_quads.reserve(quads.size() * 1.2); // 预留空间，避免多次重分配
    std::map<int, int> line_to_label_id;
    int next_label_id = 0;

    // 1. 找出所有被跳转指令引用的行号
    std::set<int> jump_target_lines;
    for (const auto& q : quads) {
        if (is_jump_op(q.op)) {
            jump_target_lines.insert(q.result.index);
        }
    }
    
    // 2. 为作为跳转目标的行生成LABEL指令，并记录行号到标签ID的映射
    for (size_t i = 0; i < quads.size(); ++i) {
        if (jump_target_lines.count(i)) {
            if (line_to_label_id.find(i) == line_to_label_id.end()) {
                line_to_label_id[i] = next_label_id;
                
                Quadruple label_quad;
                label_quad.op = OpCode::LABEL;
                label_quad.result.type = Operand::Type::LABEL;
                label_quad.result.index = next_label_id;
                // 为了方便调试，我们给标签一个名字
                label_quad.result.name = "L" + std::to_string(next_label_id);
                labeled_quads.push_back(label_quad);
                next_label_id++;
            }
        }
        labeled_quads.push_back(quads[i]);
    }

    // 3. 更新所有跳转指令，使其指向新创建的标签ID，而不是原始行号
    for (auto& q : labeled_quads) {
        if (is_jump_op(q.op)) {
            int target_line = q.result.index;
            if (line_to_label_id.count(target_line)) {
                int label_id = line_to_label_id[target_line];
                q.result.type = Operand::Type::LABEL; // 类型变更为标签
                q.result.index = label_id;
                q.result.name = "L" + std::to_string(label_id);
            }
        }
    }

    // 从这里开始，所有的优化都基于带有标签的四元式进行
    std::vector<Quadruple> optimized_quads = labeled_quads;
    
    // 1. 构建基本块
    std::vector<BasicBlock> blocks = build_basic_blocks(optimized_quads);
    
    // 2. 计算每个基本块的def和use集合
    compute_def_use_sets(blocks);
    
    // 3. 计算活跃变量
    compute_live_variables(blocks);
    
    // 4. 对基本块进行优化
    optimized_quads = optimize_basic_blocks(blocks, symbol_table);

    // 5. 重新计算跳转目标
    recompute_jump_targets(optimized_quads);
    
    return optimized_quads;
}

std::vector<BasicBlock> Optimizer::build_basic_blocks(const std::vector<Quadruple>& quads) {
    std::vector<BasicBlock> blocks;
    std::map<int, int> label_to_block; // 标签到基本块索引的映射
    
    // 第一步：识别所有基本块的入口
    std::vector<bool> is_block_entry(quads.size(), false);
    is_block_entry[0] = true; // 第一条指令总是入口
    
    for (size_t i = 0; i < quads.size(); i++) {
        const Quadruple& quad = quads[i];
        
        // 如果是跳转指令，下一条指令是入口
        if (is_jump_op(quad.op)) {
            if (i + 1 < quads.size()) {
                is_block_entry[i + 1] = true;
            }
        }
        
        // 如果是标签指令，当前指令是入口
        if (is_label_op(quad.op)) {
            is_block_entry[i] = true;
        }
    }
    
    // 第二步：构建基本块
    BasicBlock current_block;
    for (size_t i = 0; i < quads.size(); i++) {
        if (is_block_entry[i] && !current_block.quads.empty()) {
            blocks.push_back(current_block);
            current_block = BasicBlock();
        }
        
        current_block.quads.push_back(quads[i]);
        
        // 如果是跳转指令，当前基本块结束
        if (is_jump_op(quads[i].op)) {
            blocks.push_back(current_block);
            current_block = BasicBlock();
        }
    }
    
    // 添加最后一个基本块
    if (!current_block.quads.empty()) {
        blocks.push_back(current_block);
    }
    
    // 第三步：建立基本块之间的连接关系
    for (size_t i = 0; i < blocks.size(); i++) {
        const Quadruple& last_quad = blocks[i].quads.back();
        
        if (is_jump_op(last_quad.op)) {
            // 处理跳转指令
            if (last_quad.op == OpCode::JMP) {
                // 无条件跳转
                int target_label = last_quad.result.index;
                if (label_to_block.count(target_label)) {
                    blocks[i].successors.insert(label_to_block[target_label]);
                    blocks[label_to_block[target_label]].predecessors.insert(i);
                }
            } else if (last_quad.op == OpCode::JPF) {
                // 条件跳转
                int target_label = last_quad.result.index;
                if (label_to_block.count(target_label)) {
                    blocks[i].successors.insert(label_to_block[target_label]);
                    blocks[label_to_block[target_label]].predecessors.insert(i);
                }
                // 添加顺序执行的后继
                if (i + 1 < blocks.size()) {
                    blocks[i].successors.insert(i + 1);
                    blocks[i + 1].predecessors.insert(i);
                }
            }
        } else if (i + 1 < blocks.size()) {
            // 顺序执行
            blocks[i].successors.insert(i + 1);
            blocks[i + 1].predecessors.insert(i);
        }
    }
    
    return blocks;
}

void Optimizer::compute_def_use_sets(std::vector<BasicBlock>& blocks) {
    for (auto& block : blocks) {
        block.use.clear();
        block.def.clear();
        std::set<std::string> defined_in_block;
        for (const auto& quad : block.quads) {
            // Use: a variable is in USE if it's used before it's defined in the block.
            if (quad.arg1.type == Operand::Type::IDENTIFIER || quad.arg1.type == Operand::Type::TEMPORARY) {
                if (defined_in_block.find(quad.arg1.name) == defined_in_block.end()) {
                    block.use.insert(quad.arg1.name);
                }
            }
            if (quad.arg2.type == Operand::Type::IDENTIFIER || quad.arg2.type == Operand::Type::TEMPORARY) {
                if (defined_in_block.find(quad.arg2.name) == defined_in_block.end()) {
                    block.use.insert(quad.arg2.name);
                }
            }

            // Def: a variable is in DEF if it's defined in the block.
            if (quad.result.type == Operand::Type::IDENTIFIER || quad.result.type == Operand::Type::TEMPORARY) {
                block.def.insert(quad.result.name);
                defined_in_block.insert(quad.result.name);
            }
        }
    }
}

void Optimizer::compute_live_variables(std::vector<BasicBlock>& blocks) {
    bool changed;
    do {
        changed = false;
        for (auto& block : blocks) {
            // 计算新的live_out
            std::set<std::string> new_live_out;
            for (int succ : block.successors) {
                new_live_out.insert(blocks[succ].live_in.begin(), blocks[succ].live_in.end());
            }
            
            // 计算新的live_in
            std::set<std::string> new_live_in = block.use;
            std::set<std::string> temp;
            std::set_difference(new_live_out.begin(), new_live_out.end(),
                              block.def.begin(), block.def.end(),
                              std::inserter(temp, temp.begin()));
            new_live_in.insert(temp.begin(), temp.end());
            
            // 检查是否有变化
            if (new_live_in != block.live_in || new_live_out != block.live_out) {
                changed = true;
                block.live_in = new_live_in;
                block.live_out = new_live_out;
            }
        }
    } while (changed);
}

std::vector<Quadruple> Optimizer::optimize_basic_blocks(std::vector<BasicBlock>& blocks, SymbolTable& symbol_table) {
    std::vector<Quadruple> optimized_quads;
    
    for (auto& block : blocks) {
        // 在其他优化之前，先执行局部冗余存储消除
        eliminate_redundant_stores_in_block(block.quads);

        // 执行临时变量折叠，消除不必要的中间赋值
        fold_temps_in_block(block.quads, block.live_out);

        // 1. 常量折叠
        std::vector<Quadruple> optimized_block_quads;
        for (const auto& quad : block.quads) {
            if (quad.op == OpCode::ADD || quad.op == OpCode::SUB || 
                quad.op == OpCode::MUL || quad.op == OpCode::DIV) {
                if (quad.arg1.type == Operand::Type::CONSTANT && 
                    quad.arg2.type == Operand::Type::CONSTANT) {
                    // 执行常量折叠
                    double val1 = symbol_table.get_constant_table()[quad.arg1.index];
                    double val2 = symbol_table.get_constant_table()[quad.arg2.index];
                    double result;
                    
                    switch (quad.op) {
                        case OpCode::ADD: result = val1 + val2; break;
                        case OpCode::SUB: result = val1 - val2; break;
                        case OpCode::MUL: result = val1 * val2; break;
                        case OpCode::DIV: result = val2 != 0 ? val1 / val2 : 0; break;
                        default: result = 0;
                    }
                    
                    // 创建新的常量赋值四元式
                    int const_index = symbol_table.lookup_or_add_constant(result);
                    Quadruple new_quad = {
                        OpCode::ASSIGN,
                        {Operand::Type::CONSTANT, const_index, std::to_string(result)},
                        {},
                        quad.result
                    };
                    optimized_block_quads.push_back(new_quad);
                    continue;
                }
            }
            optimized_block_quads.push_back(quad);
        }
        
        // 2. 复制传播
        std::map<std::string, std::string> copy_map;
        for (const auto& quad : optimized_block_quads) {
            if (quad.op == OpCode::ASSIGN && 
                quad.arg1.type == Operand::Type::IDENTIFIER) {
                copy_map[quad.result.name] = quad.arg1.name;
            }
        }
        
        // 3. 死代码消除 (通过反向扫描基本块)
        std::vector<Quadruple> final_block_quads;
        std::set<std::string> live_vars = block.live_out; // 从块出口的活跃变量开始

        for (auto it = optimized_block_quads.rbegin(); it != optimized_block_quads.rend(); ++it) {
            const auto& quad = *it;
            bool is_dead = false;

            // 具有副作用的指令（如PRINT, CALL, JMP等）永远不被认为是死代码
            if (quad.op != OpCode::PRINT && quad.op != OpCode::CALL && !is_jump_op(quad.op) && quad.op != OpCode::RETURN) {
                // 如果指令的结果是一个临时变量
                if (quad.result.type == Operand::Type::TEMPORARY) {
                    // 并且这个临时变量在此点不是活跃的（即后续不会再被使用），则指令是死的
                    if (live_vars.find(quad.result.name) == live_vars.end()) {
                        is_dead = true;
                    }
                }
                // 如果结果是用户声明的变量(IDENTIFIER)，我们默认它总是有用的（因为它的最终值可能是程序输出），所以永远不把它当作死代码。
                // 因此，这里我们只处理TEMPORARY类型，不对IDENTIFIER做任何判断。
            }

            if (!is_dead) {
                final_block_quads.push_back(quad);

                // 更新活跃变量集合：先移除定义的变量，再添加使用的变量
                if (quad.result.type == Operand::Type::IDENTIFIER || quad.result.type == Operand::Type::TEMPORARY) {
                    live_vars.erase(quad.result.name);
                }
                if (quad.arg1.type == Operand::Type::IDENTIFIER || quad.arg1.type == Operand::Type::TEMPORARY) {
                    live_vars.insert(quad.arg1.name);
                }
                if (quad.arg2.type == Operand::Type::IDENTIFIER || quad.arg2.type == Operand::Type::TEMPORARY) {
                    live_vars.insert(quad.arg2.name);
                }
                // PRINT指令也使用它的参数
                if (quad.op == OpCode::PRINT && (quad.arg1.type == Operand::Type::IDENTIFIER || quad.arg1.type == Operand::Type::TEMPORARY)) {
                     live_vars.insert(quad.arg1.name);
                }
            }
        }
        
        // 因为是反向添加的，所以需要再次反转顺序
        std::reverse(final_block_quads.begin(), final_block_quads.end());
        
        // 将优化后的四元式添加到结果中
        optimized_quads.insert(optimized_quads.end(), 
                             final_block_quads.begin(), 
                             final_block_quads.end());
    }
    
    return optimized_quads;
}

bool Optimizer::is_jump_op(OpCode op) {
    return op == OpCode::JMP || op == OpCode::JPF;
}

bool Optimizer::is_label_op(OpCode op) {
    return op == OpCode::LABEL;
}

// 注意：每个函数的定义前都有 Optimizer::，但没有 static
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
                case OpCode::DIV: 
                    if (v2 == 0) continue;
                    result = v1 / v2; 
                    break;
                default: continue;
            }

            int const_index = symbol_table.lookup_or_add_constant(result);
            q.op = OpCode::ASSIGN;
            q.arg1 = {Operand::Type::CONSTANT, const_index, std::to_string(result)};
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
                for (const auto& expr : var_to_exprs[q.result.name]) {
                    available_exprs.erase(expr);
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
                q.op = OpCode::ASSIGN;
                q.arg1 = {Operand::Type::TEMPORARY, -1, available_exprs.at(expr)};
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
            copies.erase(q.result.name);
            for (auto it = copies.begin(); it != copies.end(); ) {
                if (it->second == q.result.name) {
                    it = copies.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        if (q.op == OpCode::ASSIGN && 
            (q.arg1.type == Operand::Type::IDENTIFIER || q.arg1.type == Operand::Type::TEMPORARY)) {
            if (q.result.name != q.arg1.name) {
                copies[q.result.name] = q.arg1.name;
            }
        }
    }
    return changed;
}

bool Optimizer::dead_code_elimination(std::vector<Quadruple>& quads, const SymbolTable& symbol_table) {
    std::set<std::string> live_vars;
    
    // 初始化活跃变量集合：我们仍然假设所有用户变量在程序末尾都是活跃的，
    // 这是一个很好的、保守的起点。
    for (const auto& entry : symbol_table.get_symbol_entries()) {
        live_vars.insert(entry.name);
    }

    size_t original_size = quads.size();
    std::vector<Quadruple> new_quads;
    new_quads.reserve(original_size);

    // 从后向前进行活跃变量分析
    for (auto it = quads.rbegin(); it != quads.rend(); ++it) {
        const auto& q = *it;
        bool is_live = false;

        // ======================= 关键修改区域 =======================
        // 检查这条指令是否需要被保留

        // 1. 有副作用的指令总是活的
        if (q.op == OpCode::JMP || q.op == OpCode::JPF || q.op == OpCode::PRINT || q.op == OpCode::CALL || q.op == OpCode::RETURN) {
            is_live = true;
        } 
        // 2. 赋值目标是用户声明的变量，则该赋值指令总是活的
        else if (q.result.type == Operand::Type::IDENTIFIER) {
            is_live = true; 
        }
        // 3. 赋值目标是临时变量，则检查该临时变量后续是否被使用
        else if (q.result.type == Operand::Type::TEMPORARY) {
            if (live_vars.count(q.result.name) > 0) {
                is_live = true;
            }
        }
        // ==========================================================

        if (is_live) {
            new_quads.push_back(q);
            // 更新活跃变量集合：live_now = (live_now - def) U use
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

    // 新列表是反向生成的，需要反转回来
    std::reverse(new_quads.begin(), new_quads.end());
    
    if (new_quads.size() != quads.size()) {
        quads = std::move(new_quads);
        return true;
    }
    
    return false;
}

void Optimizer::recompute_jump_targets(std::vector<Quadruple>& quads) {
    // 步骤 1: 建立一个从标签ID到其当前行号的映射
    std::map<int, int> label_id_to_current_line;
    for (size_t i = 0; i < quads.size(); ++i) {
        if (quads[i].op == OpCode::LABEL) {
            label_id_to_current_line[quads[i].result.index] = i;
        }
    }

    // 步骤 2: 计算如果移除LABEL指令后，每一行的最终新行号会是多少。
    // `labels_before[i]` 表示第 i 行之前（不包括第i行）有多少个LABEL指令。
    std::vector<int> labels_before(quads.size() + 1, 0);
    for (size_t i = 0; i < quads.size(); ++i) {
        labels_before[i+1] = labels_before[i];
        if (quads[i].op == OpCode::LABEL) {
            labels_before[i+1]++;
        }
    }
    
    // 计算移除所有LABEL后，最终代码的指令总数。
    const int final_quad_count = quads.size() - labels_before.back();

    // 步骤 3: 遍历所有四元式，修正跳转指令的目标
    for (auto& q : quads) {
        if (is_jump_op(q.op)) {
            // 获取跳转目标的标签ID
            int label_id = q.result.index; 

            if (label_id_to_current_line.count(label_id)) {
                // 获取标签所在的原始行（带LABEL）
                int target_line_with_labels = label_id_to_current_line[label_id];
                
                // 计算标签所在行之前有多少个LABEL。
                int num_labels_before_target = labels_before[target_line_with_labels];
                
                // 最终的目标行号 = 原始目标行号 - 前面的LABEL数量
                int final_target_line = target_line_with_labels - num_labels_before_target;
                
                q.result.index = final_target_line;
                q.result.name = std::to_string(final_target_line);
            } else {
                // 如果在标签映射表中找不到，这很可能是一个指向原始程序末尾的跳转。
                // 我们将其目标修正为优化后代码的末尾。
                q.result.index = final_quad_count;
                q.result.name = std::to_string(final_quad_count);
            }
        }
    }

    // 步骤 4: 移除所有LABEL伪指令，得到最终干净的代码
    quads.erase(std::remove_if(quads.begin(), quads.end(), [](const Quadruple& q){
        return q.op == OpCode::LABEL;
    }), quads.end());
}

void eliminate_redundant_stores_in_block(std::vector<Quadruple>& block_quads) {
    if (block_quads.empty()) {
        return;
    }

    // 记录变量最后一次定义的指令索引，这是潜在的冗余指令
    std::map<std::string, size_t> last_def_index;
    std::vector<bool> is_redundant(block_quads.size(), false);

    for (size_t i = 0; i < block_quads.size(); ++i) {
        auto& q = block_quads[i];

        // 任何对变量的读取，都会使上一次对它的赋值变得必要，不再是冗余的
        if (q.arg1.type == Operand::Type::IDENTIFIER) {
            last_def_index.erase(q.arg1.name);
        }
        if (q.arg2.type == Operand::Type::IDENTIFIER) {
            last_def_index.erase(q.arg2.name);
        }

        // 函数调用可能会以我们无法看到的方式使用任何变量，为安全起见，我们认为所有待定冗余赋值都变得必要
        if (q.op == OpCode::CALL) {
            last_def_index.clear();
        }

        // 检查对非临时变量的定义
        if (q.result.type == Operand::Type::IDENTIFIER) {
            if (last_def_index.count(q.result.name)) {
                // 找到了！前一个对该变量的赋值指令没有被使用过，因此是冗余的
                is_redundant[last_def_index.at(q.result.name)] = true;
            }
            // 记录当前这次赋值，作为下一次潜在的冗余指令
            last_def_index[q.result.name] = i;
        }
    }
    
    // 根据标记，构建新的四元式列表，排除所有冗余指令
    std::vector<Quadruple> new_quads;
    new_quads.reserve(block_quads.size());
    for (size_t i = 0; i < block_quads.size(); ++i) {
        if (!is_redundant[i]) {
            new_quads.push_back(block_quads[i]);
        }
    }
    block_quads = std::move(new_quads);
}

static void fold_temps_in_block(std::vector<Quadruple>& block_quads, const std::set<std::string>& live_out) {
    if (block_quads.empty()) {
        return;
    }
    
    std::vector<Quadruple> final_quads;
    final_quads.reserve(block_quads.size());
    std::set<std::string> live_vars = live_out;

    // 从后向前扫描基本块
    for (int i = block_quads.size() - 1; i >= 0; --i) {
        const auto& current_quad = block_quads[i];

        // 寻找模式: var := temp
        if (current_quad.op == OpCode::ASSIGN &&
            current_quad.arg1.type == Operand::Type::TEMPORARY &&
            current_quad.arg2.name.empty() &&
            // 安全性检查：temp在这次赋值后，就不再是活跃变量
            live_vars.find(current_quad.arg1.name) == live_vars.end() &&
            i > 0)
        {
            const std::string& temp_name = current_quad.arg1.name;
            const auto& prev_quad = block_quads[i-1];

            // 检查前一条指令是否是 temp := op arg1 arg2
            if ((prev_quad.op == OpCode::ADD || prev_quad.op == OpCode::SUB || prev_quad.op == OpCode::MUL || prev_quad.op == OpCode::DIV) &&
                prev_quad.result.type == Operand::Type::TEMPORARY && 
                prev_quad.result.name == temp_name) 
            {
                // 找到了可以合并的模式！
                Quadruple folded_quad = prev_quad;
                folded_quad.result = current_quad.result; // 将结果直接赋给最终变量
                final_quads.push_back(folded_quad);
                
                // 更新活跃变量集合，就好像我们处理的是合并后的指令
                if(folded_quad.result.type == Operand::Type::IDENTIFIER || folded_quad.result.type == Operand::Type::TEMPORARY) {
                    live_vars.erase(folded_quad.result.name);
                }
                if(folded_quad.arg1.type == Operand::Type::IDENTIFIER || folded_quad.arg1.type == Operand::Type::TEMPORARY) {
                    live_vars.insert(folded_quad.arg1.name);
                }
                if(folded_quad.arg2.type == Operand::Type::IDENTIFIER || folded_quad.arg2.type == Operand::Type::TEMPORARY) {
                    live_vars.insert(folded_quad.arg2.name);
                }
                
                i--; // 跳过已经被合并的前一条指令
                continue;
            }
        }
        
        final_quads.push_back(current_quad);
        
        // 为下一次迭代，正常更新活跃变量集合
        if (current_quad.result.type == Operand::Type::IDENTIFIER || current_quad.result.type == Operand::Type::TEMPORARY) {
            live_vars.erase(current_quad.result.name);
        }
        if (current_quad.arg1.type == Operand::Type::IDENTIFIER || current_quad.arg1.type == Operand::Type::TEMPORARY) {
            live_vars.insert(current_quad.arg1.name);
        }
        if (current_quad.arg2.type == Operand::Type::IDENTIFIER || current_quad.arg2.type == Operand::Type::TEMPORARY) {
            live_vars.insert(current_quad.arg2.name);
        }
    }
    
    std::reverse(final_quads.begin(), final_quads.end());
    block_quads = std::move(final_quads);
}