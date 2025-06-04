#include "CodeGenerator.h"
#include <iostream> // For debugging or simple output, to be replaced by ErrorHandler
#include "../utils/ErrorHandler.h"

// CodeGenerator::CodeGenerator(ErrorHandler& errorHandler, SymbolTable& symbolTable)
//     : error_handler_(errorHandler), symbol_table_(symbolTable) {}
CodeGenerator::CodeGenerator(ErrorHandler& errorHandler /*, SymbolTable& symbolTable*/)
    : error_handler_(errorHandler) /*, symbol_table_(symbolTable)*/ {}

std::string CodeGenerator::generate(ProgramNode* program_root) {
    output_code_stream_.str(""); // Clear previous content
    output_code_stream_.clear();  // Clear error flags
    if (!program_root) return "";

    try {
        program_root->accept(this);
    } catch (const std::runtime_error& e) {
        error_handler_.report(Error::Type::CODEGEN, "Error during code generation: " + std::string(e.what()), 
                            program_root->identifier.line_number, program_root->identifier.column_number, program_root->identifier.lexeme);
        // std::cerr << "Code Generation Error: " << e.what() << std::endl;
        // return "// Error during code generation: " + std::string(e.what());
    }
    if(error_handler_.has_errors()){
        return "// Code generation failed due to errors.";
    }
    return output_code_stream_.str();
}

// --- ASTVisitor 实现 (骨架) ---
// 在这里实现具体的代码生成逻辑
// 示例：生成类似C的代码或者某种自定义的中间代码

void CodeGenerator::visit(ProgramNode* node) {
    // Example: Start of a C-like main function or program block
    // output_code_stream_ << "// Program: " << node->identifier.lexeme << std::endl;
    // output_code_stream_ << "#include <stdio.h>" << std::endl; // If generating C
    // output_code_stream_ << "int main() {" << std::endl;

    if (node->subprogram) {
        node->subprogram->accept(this);
    }

    // output_code_stream_ << "    return 0;" << std::endl; // If generating C
    // output_code_stream_ << "}" << std::endl;
    // For a simple language as described, it might just be declarations and then statements.
}

void CodeGenerator::visit(SubprogramNode* node) {
    if (node->variable_declaration) {
        node->variable_declaration->accept(this);
        output_code_stream_ << std::endl; // Add a newline after declarations
    }
    if (node->compound_statement) {
        node->compound_statement->accept(this);
    }
}

void CodeGenerator::visit(VariableDeclarationNode* node) {
    if (!node->identifier_list || !node->type) return;

    std::string target_type = map_type_to_target(node->type->type_token.type);
    
    for (size_t i = 0; i < node->identifier_list->identifiers.size(); ++i) {
        // Assuming C-like output: "int var1, var2;"
        output_code_stream_ << "    " << target_type << " " << node->identifier_list->identifiers[i].lexeme;
        if (i < node->identifier_list->identifiers.size() -1) {
             // This part of the original grammar for <identifier_list> is a bit off for typical declarations.
             // <标识符表> -> <标识符> , <标识符表> | <标识符>
             // This usually means: id1, id2, id3. Not id1, id2, , id3
             // The parser handles it as a list of identifiers.
             // So we just declare them one by one or join them.
             // For simple C-like output, often it is `type id1, id2, id3;`
             // Let's generate separate declarations for simplicity, or join them:
             // output_code_stream_ << ", "; // if on same line
             output_code_stream_ << ";" << std::endl; // separate declarations
        } else {
             output_code_stream_ << ";" << std::endl;
        }
    }
     // Alternative for single line: 
    // output_code_stream_ << "    " << target_type << " ";
    // for (size_t i = 0; i < node->identifier_list->identifiers.size(); ++i) {
    //     output_code_stream_ << node->identifier_list->identifiers[i].lexeme;
    //     if (i < node->identifier_list->identifiers.size() - 1) {
    //         output_code_stream_ << ", ";
    //     }
    // }
    // output_code_stream_ << ";" << std::endl;
}

void CodeGenerator::visit(IdentifierListNode* node) {
    // Typically handled by parent (VariableDeclarationNode)
}

void CodeGenerator::visit(TypeNode* node) {
    // Typically handled by parent (VariableDeclarationNode), map_type_to_target does the work
}

void CodeGenerator::visit(CompoundStatementNode* node) {
    // output_code_stream_ << "    // BEGIN Compound Statement" << std::endl;
    // For C-like, 'begin' might map to '{'
    // output_code_stream_ << "    {" << std::endl; 
    // Note: The grammar does not have explicit scoping with begin/end beyond subprogram.
    // So, variables are at subprogram level. A simple begin/end block in this grammar
    // doesn't introduce a new scope for variables.

    if (node->statement_list) {
        node->statement_list->accept(this);
    }
    
    // output_code_stream_ << "    // END Compound Statement" << std::endl;
    // output_code_stream_ << "    }" << std::endl; 
}

void CodeGenerator::visit(StatementListNode* node) {
    for (const auto& stmt : node->statements) {
        if (stmt) {
            output_code_stream_ << "    "; // Indentation for statements
            stmt->accept(this);
            output_code_stream_ << ";" << std::endl; // Each assignment statement ends with a semicolon in C-like
        }
    }
}

void CodeGenerator::visit(AssignmentStatementNode* node) {
    if (!node->expression) return;

    output_code_stream_ << node->identifier.lexeme << " = ";
    node->expression->accept(this);
    // Semicolon added by StatementListNode visitor
}

void CodeGenerator::visit(ArithmeticExpressionNode* node) {
    if (!node->left) return;

    node->left->accept(this); // Left can be another ArithExpr or Term
    
    if (node->right) { // op is present
        output_code_stream_ << " " << node->op.lexeme << " ";
        node->right->accept(this); // Right is TermNode
    }
}

void CodeGenerator::visit(TermNode* node) {
    if (!node->left) return;

    node->left->accept(this); // Left can be TermNode or FactorNode

    if (node->right) { // op is present
        output_code_stream_ << " " << node->op.lexeme << " ";
        node->right->accept(this); // Right is FactorNode
    }
}

void CodeGenerator::visit(FactorNode* node) {
    if (!node->content) return;

    if (node->is_parenthesized) {
        output_code_stream_ << "(";
    }
    node->content->accept(this);
    if (node->is_parenthesized) {
        output_code_stream_ << ")";
    }
}

void CodeGenerator::visit(VariableNode* node) {
    output_code_stream_ << node->identifier.lexeme;
}

void CodeGenerator::visit(ConstantNode* node) {
    output_code_stream_ << node->constant_token.lexeme;
}

// --- 辅助函数实现 ---
std::string CodeGenerator::new_label() {
    return "L" + std::to_string(label_counter_++);
}

std::string CodeGenerator::map_type_to_target(TokenType type) {
    switch (type) {
        case TokenType::INTEGER: return "int";
        case TokenType::REAL:    return "double"; // or float
        case TokenType::CHAR:    return "char";
        // case TokenType::INTEGER_CONST: return "int"; // This is a value type, not declaration type
        // case TokenType::REAL_CONST:    return "double";
        default:
            error_handler_.report(Error::Type::CODEGEN, "Cannot map unknown TokenType to target language type: " + Token(type,"",0,0).toString());
            throw std::runtime_error("Cannot map unknown TokenType to target language type.");
    }
} 