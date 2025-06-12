#ifndef TARGET_CODE_GENERATOR_H
#define TARGET_CODE_GENERATOR_H

#include <vector>
#include <string>
#include "../parser/Quadruple.h"
#include "../semantic_analyzer/SymbolTable.h"

// 为了传递活跃变量分析的结果
struct TargetCodeLine {
    int line_number;
    std::string code;
    // 如果需要，未来可以添加更多信息，比如对应哪个四元式
};

class TargetCodeGenerator {
public:
    TargetCodeGenerator();

    /**
     * @brief 生成目标代码的主要函数
     * @param quads 优化后的四元式序列
     * @param symbol_table 符号表，用于查找常量等信息
     * @return 一个包含目标代码行的向量
     */
    std::vector<TargetCodeLine> generate(const std::vector<Quadruple>& quads, SymbolTable& symbol_table);

private:
    // 目标代码生成逻辑可以作为私有成员函数
};

#endif // TARGET_CODE_GENERATOR_H 