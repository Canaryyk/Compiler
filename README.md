# Simple Compiler

这是一个基于C++实现的简单编译器项目。

## 文法产生式

下面是编译器所实现的语言的文法规则:

-   `{ ... }` 表示重复0次或多次。
-   `[ ... ]` 表示可选（0次或1次）。
-   `|` 表示选择。

### 语法规则 (Syntax Rules)

```
<Program>           ::= 'program' <Identifier> <Block> '.'
// <程序>           ::= 'program' <标识符> <分程序> '.'

<Block>             ::= <Declarations> <CompoundStatement>
// <分程序>         ::= <说明部分> <复合语句>

<Declarations>      ::= [ <VarDeclarations> ] [ <SubprogramDeclarations> ]
// <说明部分>      ::= [ <变量说明> ] [ <子程序说明部分> ]

<SubprogramDeclarations> ::= { <ProcedureDeclaration> | <FunctionDeclaration> }
// <子程序说明部分> ::= { <过程说明> ';' | <函数说明> ';' }

<VarDeclarations>   ::= 'var' <VarDeclaration> { <VarDeclaration> }
// <变量说明>       ::= 'var' <变量声明> { <变量声明> }

<VarDeclaration>    ::= <IdentifierList> ':' <Type> ';'
// <变量声明>       ::= <标识符表> ':' <类型> ';'
// 语义动作:        { 将变量信息登录到符号表 }

<IdentifierList>    ::= <Identifier> { ',' <Identifier> }
// <标识符表>       ::= <标识符> { ',' <标识符> }

<Type>              ::= 'integer' | 'real'
// <类型>            ::= 'integer' | 'real'

<ProcedureDeclaration> ::= 'procedure' <Identifier> [ '(' <ParameterList> ')' ] ';' <Block> ';'
// <过程声明>       ::= 'procedure' <标识符> [ '(' <形参列表> ')' ] ';' <分程序> ';'
// 语义动作:        { 1. 创建新符号表项, 记录过程名和层级。
//                   2. 创建新作用域。
//                   3. 处理形参列表, 将形参登录到新作用域的符号表。
//                   4. 递归调用 Block 解析过程体。
//                   5. 退出作用域。 }

<FunctionDeclaration>  ::= 'function' <Identifier> [ '(' <ParameterList> ')' ] ':' <Type> ';' <Block> ';'
// <函数声明>       ::= 'function' <标识符> [ '(' <形参列表> ')' ] ':' <类型> ';' <分程序> ';'
// 语义动作:        { 与过程类似, 但需额外记录返回类型。}

<ParameterList>      ::= <Parameter> { ';' <Parameter> }
// <形参列表>       ::= <形参> { ';' <形参> }

<Parameter>          ::= <IdentifierList> ':' <Type>
// <形参>           ::= <标识符表> ':' <类型>

<CompoundStatement> ::= 'begin' <StatementList> 'end'
// <复合语句>       ::= 'begin' <语句表> 'end'

<StatementList>     ::= <Statement> { ';' <Statement> }
// <语句表>         ::= <语句> { ';' <语句> }

<Statement>         ::= <AssignmentStatement>
                    |   <IfStatement>
                    |   <WhileStatement>
                    |   <CompoundStatement>
                    |   <SubprogramCall>
                    |   ε
// <语句>            ::= <赋值语句> | <条件语句> | <循环语句> | <复合语句> | <子程序调用> | ε

<AssignmentStatement> ::= <Identifier> ':=' <Expression>
// <赋值语句>       ::= <标识符> ':=' <表达式>
// 语义动作:        { 1. 生成 (ASSIGN, Expression.result, -, Identifier.place) 四元式。
//                   2. 若赋值目标为函数名, 则此赋值视为函数返回值, 生成 (RETURN, Expression.result, -, -) }

<IfStatement>       ::= 'if' <Condition> 'then' <Statement> [ 'else' <Statement> ]
// <条件语句>       ::= 'if' <条件> 'then' <语句> [ 'else' <语句> ]
// 语义动作:        { 1. 在 <Condition> 后, 生成带假出口的跳转指令 JPF。
//                   2. 在 then <Statement> 后, 若有 else, 生成无条件跳转指令 JMP, 并回填 JPF 的目标到 else 子句。
//                   3. 在整个语句结束后, 回填所有未确定的跳转目标地址。 }

<WhileStatement>    ::= 'while' <Condition> 'do' <Statement>
// <循环语句>       ::= 'while' <条件> 'do' <语句>
// 语义动作:        { 1. 记录循环开始地址。
//                   2. 在 <Condition> 后, 生成带假出口的跳转指令 JPF。
//                   3. 在 <Statement> 后, 生成无条件跳转指令跳回循环开始处。
//                   4. 回填 JPF 的目标地址到循环结束之后。 }

<SubprogramCall>    ::= <Identifier> '(' [ <ArgumentList> ] ')'
// <子程序调用>   ::= <标识符> '(' [ <实参列表> ] ')'
// 语义动作:        { 1. 查找符号表, 验证其为函数或过程。
//                   2. 处理实参列表, 生成 PARAM 四元式。
//                   3. 检查实参与形参的匹配性。
//                   4. 生成 (CALL, callee, arg_count, result_temp) 四元式。 }

<ArgumentList>      ::= <Expression> { ',' <Expression> }
// <实参列表>       ::= <表达式> { ',' <表达式> }

<Condition>         ::= <Expression> <RelationalOp> <Expression>
// <条件>            ::= <表达式> <关系运算符> <表达式>
// 语义动作:        { t = new_temp(); 生成 (RelationalOp, E1.result, E2.result, t); Condition.result = t }

<Expression>        ::= <Term> { ( '+' | '-' ) <Term> }
// <表达式>         ::= <项> { ( '+' | '-' ) <项> }
// 语义动作:        { 对于每个 (+|-) <Term>, t = new_temp(); 生成 (op, E.result, T.result, t); E.result = t }

<Term>              ::= <Factor> { ( '*' | '/' ) <Factor> }
// <项>              ::= <因子> { ( '*' | '/' ) <因子> }
// 语义动作:        { 对于每个 (*|/) <Factor>, t = new_temp(); 生成 (op, T.result, F.result, t); T.result = t }

<Factor>            ::= <Identifier> | <Constant> | '(' <Expression> ')' | <SubprogramCall>
// <因子>            ::= <标识符> | <常数> | '(' <表达式> ')' | <子程序调用>

<RelationalOp>      ::= '=' | '<>' | '<' | '<=' | '>' | '>='
// <关系运算符>      ::= '=' | '<>' | '<' | '<=' | '>' | '>='
```

### 词法规则 (Lexical Rules)

这部分定义了构成语言的基本符号（单词）的结构。

```
<Identifier>   ::= <Letter> { <Letter> | <Digit> }
// <标识符>      ::= <字母> { <字母> | <数字> }

<Constant>     ::= <Integer> | <Real>
// <常数>        ::= <整数> | <实数>

<Integer>      ::= <Digit> { <Digit> }
// <整数>        ::= <数字> { <数字> }

<Real>         ::= <Integer> '.' <Integer>
// <实数>        ::= <整数> '.' <整数>

<Letter>       ::= a | b | ... | z | A | B | ... | Z
// <字母>        ::= a | b | ... | z | A | B | ... | Z

<Digit>        ::= 0 | 1 | ... | 9
// <数字>        ::= 0 | 1 | ... | 9
```