# Simple Compiler

这是一个基于C++实现的简单编译器项目。

## 项目结构

```
Compiler/
├── src/
│   ├── main.cpp
│   ├── lexer/
│   │   ├── Token.h
│   │   ├── Lexer.h
│   │   └── Lexer.cpp
│   ├── parser/
│   │   ├── AST.h
│   │   ├── Parser.h
│   │   └── Parser.cpp
│   ├── semantic_analyzer/
│   │   ├── SymbolTable.h
│   │   ├── SemanticAnalyzer.h
│   │   └── SemanticAnalyzer.cpp
│   ├── code_generator/
│   │   ├── CodeGenerator.h
│   │   └── CodeGenerator.cpp
│   └── utils/
│       ├── ErrorHandler.h
│       └── ErrorHandler.cpp
├── tests/           # 测试用例
├── examples/        # 示例代码
├── CMakeLists.txt   # CMake 构建配置
└── README.md        # 项目说明
```

## 构建指南

1.  确保已安装 CMake (版本 >= 3.10) 和 C++ 编译器 (支持 C++17)。
2.  创建 build 目录并进入: `mkdir build && cd build`
3.  运行 CMake: `cmake ..`
4.  编译项目: `cmake --build .` (或者 `make`)

## 使用

编译完成后，可执行文件 `SimpleCompiler` (或 `SimpleCompiler.exe`) 会生成在 `build` 目录下 (或者对应平台的子目录)。

```bash
./SimpleCompiler <source_file_path>
```

## 待办

- [ ] 完善词法分析器 (例如，支持更多数据类型、关键字、操作符)
- [ ] 完善语法分析器 (例如，支持更复杂的表达式、语句、控制流结构)
- [ ] 完善抽象语法树 (AST) 结构和节点类型
- [ ] 完善语义分析器 (例如，增强类型检查、作用域管理、错误报告)
- [ ] 完善代码生成器 (例如，支持生成特定目标平台的汇编代码或虚拟机指令)
- [x] 初步集成各编译器模块
- [ ] 添加全面的单元测试和集成测试
- [ ] 增强错误处理机制和错误恢复能力
- [ ] 编写更详细的开发文档和用户文档
- [ ] 考虑支持更多语言特性 (例如，函数、数组、结构体等)
