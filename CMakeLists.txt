cmake_minimum_required(VERSION 3.10)
project(SimpleCompiler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置源文件目录
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 添加可执行文件
add_executable(SimpleCompiler ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/lexer/Lexer.cpp
    ${SOURCE_DIR}/parser/Parser.cpp
    ${SOURCE_DIR}/semantic_analyzer/SemanticAnalyzer.cpp
    ${SOURCE_DIR}/code_generator/CodeGenerator.cpp
    ${SOURCE_DIR}/utils/ErrorHandler.cpp
)

# 添加头文件目录 (如果使用 include 目录)
# target_include_directories(SimpleCompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 可以在这里添加单元测试等

# 输出编译信息
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}") 