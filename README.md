# Simple Compiler

这是一个基于C++实现的简单编译器项目。

## 文法产生式

下面是编译器所实现的语言的文法规则，采用类EBNF范式书写。

-   `{ ... }` 表示重复0次或多次。
-   `[ ... ]` 表示可选（0次或1次）。
-   `|` 表示选择。

### 语法规则 (Syntax Rules)

下面是编译器所实现的语言的文法规则，采用类EBNF范式书写。

-   `{ ... }` 表示重复0次或多次。
-   `[ ... ]` 表示可选（0次或1次）。
-   `|` 表示选择。

### 语法规则 (Syntax Rules)

```
<Program>           ::= 'program' <Identifier> <Block> '.'
// <程序>           ::= 'program' <标识符> <分程序> '.'
<Program>           ::= 'program' <Identifier> <Block> '.'
// <程序>           ::= 'program' <标识符> <分程序> '.'

<Block>             ::= [ <VarDeclarations> ] <CompoundStatement>
// <分程序>         ::= [ <变量说明> ] <复合语句>

<VarDeclarations>   ::= 'var' <VarDeclaration> { <VarDeclaration> }
// <变量说明>       ::= 'var' <变量声明> { <变量声明> }

<VarDeclaration>    ::= <IdentifierList> ':' <Type> ';'
// <变量声明>       ::= <标识符表> ':' <类型> ';'
// 语义动作:        { 将变量信息登录到符号表 }
<Block>             ::= [ <VarDeclarations> ] <CompoundStatement>
// <分程序>         ::= [ <变量说明> ] <复合语句>

<VarDeclarations>   ::= 'var' <VarDeclaration> { <VarDeclaration> }
// <变量说明>       ::= 'var' <变量声明> { <变量声明> }

<VarDeclaration>    ::= <IdentifierList> ':' <Type> ';'
// <变量声明>       ::= <标识符表> ':' <类型> ';'
// 语义动作:        { 将变量信息登录到符号表 }

<IdentifierList>    ::= <Identifier> { ',' <Identifier> }
// <标识符表>       ::= <标识符> { ',' <标识符> }

<Type>              ::= 'integer' | 'real'
// <类型>            ::= 'integer' | 'real'
<IdentifierList>    ::= <Identifier> { ',' <Identifier> }
// <标识符表>       ::= <标识符> { ',' <标识符> }

<Type>              ::= 'integer' | 'real'
// <类型>            ::= 'integer' | 'real'

<CompoundStatement> ::= 'begin' <StatementList> 'end'
// <复合语句>       ::= 'begin' <语句表> 'end'

<StatementList>     ::= <Statement> { ';' <Statement> }
// <语句表>         ::= <语句> { ';' <语句> }

<Statement>         ::= <AssignmentStatement>
                    |   <IfStatement>
                    |   <WhileStatement>
                    |   <CompoundStatement>
                    |   ε
// <语句>            ::= <赋值语句> | <条件语句> | <循环语句> | <复合语句> | ε
<CompoundStatement> ::= 'begin' <StatementList> 'end'
// <复合语句>       ::= 'begin' <语句表> 'end'

<StatementList>     ::= <Statement> { ';' <Statement> }
// <语句表>         ::= <语句> { ';' <语句> }

<Statement>         ::= <AssignmentStatement>
                    |   <IfStatement>
                    |   <WhileStatement>
                    |   <CompoundStatement>
                    |   ε
// <语句>            ::= <赋值语句> | <条件语句> | <循环语句> | <复合语句> | ε

<AssignmentStatement> ::= <Identifier> ':=' <Expression>
// <赋值语句>       ::= <标识符> ':=' <表达式>
// 语义动作:        { 生成 (ASSIGN, Expression.result, -, Identifier.place) 四元式 }

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

<Condition>         ::= <Expression> <RelationalOp> <Expression>
// <条件>            ::= <表达式> <关系运算符> <表达式>
// 语义动作:        { t = new_temp(); 生成 (RelationalOp, E1.result, E2.result, t); Condition.result = t }

<Expression>        ::= <Term> { ( '+' | '-' ) <Term> }
// <表达式>         ::= <项> { ( '+' | '-' ) <项> }
// 语义动作:        { 对于每个 (+|-) <Term>, t = new_temp(); 生成 (op, E.result, T.result, t); E.result = t }

<Term>              ::= <Factor> { ( '*' | '/' ) <Factor> }
// <项>              ::= <因子> { ( '*' | '/' ) <因子> }
// 语义动作:        { 对于每个 (*|/) <Factor>, t = new_temp(); 生成 (op, T.result, F.result, t); T.result = t }

<Factor>            ::= <Identifier> | <Constant> | '(' <Expression> ')'
// <因子>            ::= <标识符> | <常数> | '(' <表达式> ')'

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


