# Simple Compiler

这是一个基于C++实现的简单编译器项目。

## 文法产生式

### 1. 语言的文法产生式集合

#### 程序定义
```
<Program> → program <Identifier> <Block> .
对应中文产生式: <程序> → program <标识符> <分程序> .

<Block> → <VarDeclarations> <CompoundStatement>
对应中文产生式: <分程序> → <变量说明> <复合语句>
```

#### 语句定义
```
<VarDeclarations> → var <IdentifierList> : <Type> ;
对应中文产生式: <变量说明> → var <标识符表> : <类型> ;

<IdentifierList> → <Identifier> | <Identifier> , <IdentifierList>
对应中文产生式: <标识符表> → <标识符> | <标识符> , <标识符表>

<CompoundStatement> → begin <StatementList> end
对应中文产生式: <复合语句> → begin <语句表> end

<StatementList> → <Statement> <StatementListTail>
对应中文产生式: <语句表> → <语句> <语句表后缀>

<StatementListTail> → ; <StatementList> | ε
对应中文产生式: <语句表后缀> → ; <语句表> | ε

<Statement> → <AssignmentStatement> | <IfStatement> | <WhileStatement> | <CompoundStatement> | ε
对应中文产生式: <语句> → <赋值语句> | <条件语句> | <循环语句> | <复合语句> | ε

<AssignmentStatement> → <Identifier> := <Expression>
对应中文产生式: <赋值语句> → <标识符> := <算术表达式>

<IfStatement> → if <Condition> then <Statement> | if <Condition> then <Statement> else <Statement>
对应中文产生式: <条件语句> → if <条件表达式> then <语句> | if <条件表达式> then <语句> else <语句>

<WhileStatement> → while <Condition> do <Statement>
对应中文产生式: <循环语句> → while <条件表达式> do <语句>
```
#### 条件表达式定义
```
<Condition> → <Expression> <RelationalOp> <Expression>
对应中文产生式: <条件表达式> → <算术表达式> <关系运算符> <算术表达式>

<RelationalOp> → = | <> | < | <= | > | >=
对应中文产生式: <关系运算符> → = | <> | < | <= | > | >=
```

#### 算术表达式定义
```
<Expression> → <Term> <ExpressionTail>
对应中文产生式: <算术表达式> → <项> <算术表达式后缀>

<ExpressionTail> → + <Term> <ExpressionTail> | - <Term> <ExpressionTail> | ε
对应中文产生式: <算术表达式后缀> → + <项> <算术表达式后缀> | - <项> <算术表达式后缀> | ε

<Term> → <Factor> <TermTail>
对应中文产生式: <项> → <因子> <项后缀>

<TermTail> → * <Factor> <TermTail> | / <Factor> <TermTail> | ε
对应中文产生式: <项后缀> → * <因子> <项后缀> | / <因子> <项后缀> | ε

<Factor> → <Value> | ( <Expression> )
对应中文产生式: <因子> → <算术量> | ( <算术表达式> )

<Value> → <Identifier> | <Constant>
对应中文产生式: <算术量> → <标识符> | <常数>
```

#### 类型定义
```
<Type> → integer | real | char
对应中文产生式: <类型> → integer | real | char
```

#### 单词集定义
```
<Identifier> → <Letter> <IdentifierTail>
对应中文产生式: <标识符> → <字母> <标识符后缀>

<IdentifierTail> → <Digit> <IdentifierTail> | <Letter> <IdentifierTail> | ε
对应中文产生式: <标识符后缀> → <数字> <标识符后缀> | <字母> <标识符后缀> | ε

<Constant> → <Integer> | <Real>
对应中文产生式: <常数> → <整数> | <实数>

<Integer> → <Digit> <IntegerTail>
对应中文产生式: <整数> → <数字> <整数后缀>

<IntegerTail> → <Digit> <IntegerTail> | ε
对应中文产生式: <整数后缀> → <数字> <整数后缀> | ε

<Real> → <Integer> . <Integer>
对应中文产生式: <实数> → <整数> . <整数>
```

#### 字符集定义
```
<Letter> → A | B | ... | Z | a | b | ... | z
对应中文产生式: <字母> → A | B | ... | Z | a | b | ... | z

<Digit> → 0 | 1 | ... | 9
对应中文产生式: <数字> → 0 | 1 | ... | 9
```

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
└── README.md        # 项目说明
```

## 文件和目录说明

以下是项目中主要文件和目录的详细说明：

*   **`.git/`**:
    *   **说明**: Git 版本控制目录，包含项目的版本历史和元数据。
    *   **注意**: 不应直接修改此目录下的文件。
*   **`README.md`** (本文档):
    *   **说明**: 项目的入口文档，提供了项目的整体概览、详细的文法产生式定义、清晰的项目结构图、各个文件和目录的用途说明、具体的使用方法以及未来的待办事项列表。
*   **`test`**:
    *   **说明**: 一个空文件。
    *   **目的**: 可能作为特定测试脚本的占位符或用于其他与测试相关的目的。
*   **`src/`**:
    *   **说明**: 存放编译器核心源代码的目录。所有主要的编译逻辑和模块都位于此目录下。
    *   **`src/main.cpp`**:
        *   **说明**: 编译器的主入口文件。
        *   **功能**:
            *   解析命令行参数。
            *   初始化编译器核心组件（词法分析器、语法分析器、语义分析器、代码生成器）。
            *   按顺序调用这些组件来处理输入的源代码文件，完成编译流程。
    *   **`src/code_generator/`**:
        *   **说明**: 存放代码生成器模块的代码。
        *   **功能**: 此模块负责将抽象语法树（AST）或中间表示（IR）转换为目标机器代码或另一种高级语言。
        *   **`CodeGenerator.h`**:
            *   **说明**: 代码生成器模块的头文件。
            *   **内容**: 通常包含 `CodeGenerator` 类的声明，定义其接口、成员变量和方法。这些接口供其他模块（主要是 `main.cpp`）调用。
        *   **`CodeGenerator.cpp`**:
            *   **说明**: 代码生成器模块的实现文件。
            *   **内容**: 包含 `CodeGenerator` 类方法的具体实现，以及执行代码生成逻辑所需的任何辅助函数或数据结构，例如指令选择、寄存器分配（如果适用）和最终目标代码的格式化输出。
    *   **`src/lexer/`**:
        *   **说明**: 存放词法分析器（也称为扫描器）模块的代码。
        *   **功能**: 此模块负责读取源代码文本，并将其分解成一系列的词法单元（tokens）。
        *   **`Token.h`**:
            *   **说明**: 定义词法单元（Token）的结构或类。
            *   **内容**: 通常包含 Token 的以下信息：
                *   类型（例如：关键字、标识符、操作符、字面量等）。
                *   值（例如：标识符的名称、数字字面量的值）。
                *   在源代码中的位置信息（行号、列号），这对于后续的语法分析和错误报告非常重要。
        *   **`Lexer.h`**:
            *   **说明**: 词法分析器模块的头文件。
            *   **内容**: 通常包含 `Lexer` 类的声明，定义其接口（例如：获取下一个 Token 的方法）以及可能的状态管理变量。
        *   **`Lexer.cpp`**:
            *   **说明**: 词法分析器模块的实现文件。
            *   **内容**: 包含 `Lexer` 类方法的具体实现，负责读取输入字符流，根据预定义的词法规则识别和构建 Token 序列，并处理注释和空白字符。
    *   **`src/parser/`**:
        *   **说明**: 存放语法分析器模块的代码。
        *   **功能**: 此模块负责接收词法分析器生成的词法单元流，并根据定义的语法规则构建抽象语法树（AST）。
        *   **`AST.h`**:
            *   **说明**: 定义抽象语法树（AST）的节点类型和结构。
            *   **内容**: 包含各种 AST 节点（例如：表达式节点、语句节点、声明节点等）的类或结构体声明，以及它们之间的关系（例如：父子关系、兄弟关系）。AST 是编译器后续阶段（如语义分析、代码生成）进行处理的基础数据结构。
        *   **`Parser.h`**:
            *   **说明**: 语法分析器模块的头文件。
            *   **内容**: 通常包含 `Parser` 类的声明，定义其接口，例如开始解析并返回 AST 根节点的方法。
        *   **`Parser.cpp`**:
            *   **说明**: 语法分析器模块的实现文件。
            *   **内容**: 包含 `Parser` 类方法的具体实现，应用特定的语法分析算法（例如：递归下降、LL/LR 等）来解析 Token 流，构建 AST，并报告遇到的语法错误。
    *   **`src/semantic_analyzer/`**:
        *   **说明**: 存放语义分析器模块的代码。
        *   **功能**: 此模块负责检查 AST 的语义正确性，确保代码符合语言的语义规则。
        *   **`SymbolTable.h`**:
            *   **说明**: 符号表模块的头文件。
            *   **内容**: 定义符号表（`SymbolTable`）的类或结构，用于存储程序中声明的标识符（如变量名、函数名）及其相关信息（如类型、作用域、内存位置等）。它通常提供插入、查找和管理作用域等操作的接口。
        *   **`SymbolTable.cpp`**:
            *   **说明**: 符号表模块的实现文件。
            *   **内容**: 包含 `SymbolTable` 类方法的具体实现。
        *   **`SemanticAnalyzer.h`**:
            *   **说明**: 语义分析器模块的头文件。
            *   **内容**: 通常包含 `SemanticAnalyzer` 类的声明，定义其接口，例如遍历 AST 并执行语义检查的方法。
        *   **`SemanticAnalyzer.cpp`**:
            *   **说明**: 语义分析器模块的实现文件。
            *   **内容**: 包含 `SemanticAnalyzer` 类方法的具体实现，负责执行类型检查、作用域解析、变量声明与使用匹配等语义规则，并更新符号表。
    *   **`src/utils/`**:
        *   **说明**: 存放一些工具类或辅助函数，供编译器其他模块共享使用。
        *   **`ErrorHandler.h`**:
            *   **说明**: 错误处理器模块的头文件。
            *   **内容**: 通常声明错误报告相关的函数或类，提供统一的接口来记录和显示编译过程中发现的词法错误、语法错误和语义错误。可能包括错误代码、错误信息格式化、错误位置指示等功能。
        *   **`ErrorHandler.cpp`**:
            *   **说明**: 错误处理器模块的实现文件。
            *   **内容**: 包含错误报告功能的具体实现。
*   **`tests/`**:
    *   **说明**: 存放测试代码的目录。
    *   **`tests/.gitkeep`**:
        *   **说明**: 一个空文件。
        *   **目的**: 用于确保 Git 会跟踪这个空目录，即使它最初没有任何测试文件。
    *   **职责**:
        *   包含用于验证编译器各个模块功能正确性的单元测试、集成测试和回归测试。
    *   **应包含代码**:
        *   测试框架的配置文件。
        *   针对词法分析器、语法分析器、语义分析器、代码生成器等模块的详细测试用例。
*   **`examples/`**:
    *   **说明**: 存放示例代码的目录。
    *   **`examples/.gitkeep`**:
        *   **说明**: 一个空文件。
        *   **目的**: 确保 Git 跟踪此空目录，即使它最初没有任何示例文件。
    *   **职责**:
        *   包含一些使用该编译器进行编译的简单示例程序。
        *   演示特定语言特性的代码片段。
        *   这些示例可以帮助用户理解编译器的功能和用法，也可以作为测试编译器的实际输入。
    *   **应包含代码**:
        *   符合编译器所支持语言语法的源代码文件（例如 `.txt`, `.lang`, `.src` 等，取决于编译器定义）。

