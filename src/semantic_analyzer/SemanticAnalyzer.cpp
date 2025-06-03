#include "SemanticAnalyzer.h"
#include <iostream> // For temporary error/info messages, will be removed or conditional
#include <functional> // Required for std::function
#include "../utils/ErrorHandler.h"

SemanticAnalyzer::SemanticAnalyzer(ErrorHandler& errorHandler)
    : error_handler_(errorHandler) {}
// SemanticAnalyzer::SemanticAnalyzer(/*ErrorHandler& errorHandler*/) {
    // 初始化，例如可以预定义一些内建类型到符号表（如果需要）
// }

void SemanticAnalyzer::analyze(ProgramNode* program_root) {
    if (!program_root) return;
    try {
        program_root->accept(this);
    } catch (const std::runtime_error& e) {
        // Errors should be reported using error_handler_ before throwing.
        // This catch is a fallback or for unexpected issues.
        // error_handler_.report_semantic_error(program_root->identifier, e.what()); // Example, needs better context
        // std::cerr << "Semantic Error (Caught in analyze): " << e.what() << std::endl;
    }
}

// --- ASTVisitor 实现 (骨架) ---
// 在这里实现具体的语义检查逻辑

void SemanticAnalyzer::visit(ProgramNode* node) {
    symbol_table_.enter_scope(); 
    if (node->subprogram) {
        node->subprogram->accept(this);
    }
    symbol_table_.exit_scope(); 
}

void SemanticAnalyzer::visit(SubprogramNode* node) {
    if (node->variable_declaration) {
        node->variable_declaration->accept(this);
    }
    if (node->compound_statement) {
        node->compound_statement->accept(this);
    }
}

void SemanticAnalyzer::visit(VariableDeclarationNode* node) {
    if (!node->identifier_list || !node->type) {
        error_handler_.report_semantic_error(node->var_keyword, "Malformed variable declaration.");
        throw std::runtime_error("Malformed variable declaration."); 
    }
    
    node->type->accept(this); 
    TokenType var_type = node->type->type_token.type;
    if (var_type != TokenType::INTEGER && var_type != TokenType::REAL && var_type != TokenType::CHAR) {
         error_handler_.report_semantic_error(node->type->type_token, "Invalid variable type token in declaration.");
         throw std::runtime_error("Invalid variable type token in declaration.");
    }

    for (const auto& identifier_token : node->identifier_list->identifiers) {
        if (!symbol_table_.insert(identifier_token.lexeme, SymbolType::VARIABLE, var_type)) {
            error_handler_.report_semantic_error(identifier_token, "Variable '" + identifier_token.lexeme + "' already declared in this scope.");
            throw std::runtime_error("Variable '" + identifier_token.lexeme + "' already declared in this scope.");
        }
    }
}

void SemanticAnalyzer::visit(IdentifierListNode* node) {
    // Handled by parent
}

void SemanticAnalyzer::visit(TypeNode* node) {
    TokenType type = node->type_token.type;
    if (type != TokenType::INTEGER && type != TokenType::REAL && type != TokenType::CHAR) {
        error_handler_.report_semantic_error(node->type_token, "Unknown type '" + node->type_token.lexeme + "' specified.");
        throw std::runtime_error("Unknown type '" + node->type_token.lexeme + "' specified.");
    }
}

void SemanticAnalyzer::visit(CompoundStatementNode* node) {
    if (node->statement_list) {
        node->statement_list->accept(this);
    }
}

void SemanticAnalyzer::visit(StatementListNode* node) {
    for (const auto& stmt : node->statements) {
        if (stmt) {
            stmt->accept(this);
        }
    }
}

void SemanticAnalyzer::visit(AssignmentStatementNode* node) {
    auto symbol = symbol_table_.lookup(node->identifier.lexeme);
    if (!symbol) {
        error_handler_.report_semantic_error(node->identifier, "Undeclared variable '" + node->identifier.lexeme + "' in assignment.");
        throw std::runtime_error("Undeclared variable '" + node->identifier.lexeme + "' in assignment.");
    }
    if (symbol->symbol_type != SymbolType::VARIABLE) {
        error_handler_.report_semantic_error(node->identifier, "Identifier '" + node->identifier.lexeme + "' is not a variable.");
        throw std::runtime_error("Identifier '" + node->identifier.lexeme + "' is not a variable.");
    }

    if (!node->expression) {
        error_handler_.report_semantic_error(node->assign_op, "Missing expression in assignment statement for '" + node->identifier.lexeme + "'.");
        throw std::runtime_error("Missing expression in assignment statement for '" + node->identifier.lexeme + "'.");
    }
    node->expression->accept(this); 
    TokenType expr_type = get_expression_type(node->expression.get());
    TokenType var_type = symbol->data_type;

    bool compatible = false;
    if (var_type == expr_type) {
        compatible = true;
    } else if (var_type == TokenType::REAL && expr_type == TokenType::INTEGER_CONST) { 
        compatible = true;
    } else if (var_type == TokenType::INTEGER && expr_type == TokenType::INTEGER_CONST){
        compatible = true; // INTEGER var = INTEGER_CONST
    } else if (var_type == TokenType::REAL && expr_type == TokenType::REAL_CONST){
        compatible = true; // REAL var = REAL_CONST
    } else if (var_type == TokenType::CHAR && expr_type == TokenType::CHAR){ // Assuming CHAR literal gives CHAR type
        compatible = true;
    }

    if (!compatible) {
        std::string var_type_str = Token(var_type, "",0,0).toString(); // Helper to get string name
        std::string expr_type_str = Token(expr_type, "",0,0).toString();
        error_handler_.report_semantic_error(node->assign_op, 
            "Type mismatch in assignment to '" + node->identifier.lexeme + 
            "'. Expected compatible type for " + var_type_str + 
            " but got " + expr_type_str);
        throw std::runtime_error("Type mismatch in assignment to '" + node->identifier.lexeme + "'.");
    }
}

void SemanticAnalyzer::visit(ArithmeticExpressionNode* node) {
    if (node->left) node->left->accept(this);
    if (node->right) node->right->accept(this); 
    // Type determination is done in get_expression_type, which is called by parent (e.g. Assignment)
}

void SemanticAnalyzer::visit(TermNode* node) {
    if (node->left) node->left->accept(this);
    if (node->right) node->right->accept(this); 
}

void SemanticAnalyzer::visit(FactorNode* node) {
    if (node->content) {
        node->content->accept(this);
    }
}

void SemanticAnalyzer::visit(VariableNode* node) {
    auto symbol = symbol_table_.lookup(node->identifier.lexeme);
    if (!symbol) {
        error_handler_.report_semantic_error(node->identifier, "Undeclared variable '" + node->identifier.lexeme + "' used in expression.");
        throw std::runtime_error("Undeclared variable '" + node->identifier.lexeme + "' used in expression.");
    }
    if (symbol->symbol_type != SymbolType::VARIABLE) {
        error_handler_.report_semantic_error(node->identifier, "Identifier '" + node->identifier.lexeme + "' is not a variable, used in expression.");
        throw std::runtime_error("Identifier '" + node->identifier.lexeme + "' is not a variable, used in expression.");
    }
}

void SemanticAnalyzer::visit(ConstantNode* node) {
    // Constants are inherently typed by the lexer/parser.
    // No specific check needed here unless we add attributes to ConstantNode for its type.
}

// --- 辅助函数实现 ---
TokenType SemanticAnalyzer::get_expression_type(ArithmeticExpressionNode* expr_node) {
    if (!expr_node) {
        error_handler_.report(Error::Type::SEMANTIC, "Cannot get type of null expression node.");
        throw std::runtime_error("Cannot get type of null expression node.");
    }

    std::function<TokenType(ASTNode*)> getTypeRecursive = 
        [&](ASTNode* current_node) -> TokenType {
        if (!current_node) {
            error_handler_.report(Error::Type::SEMANTIC, "Null node in get_expression_type traversal");
            throw std::runtime_error("Null node in get_expression_type traversal");
        }

        if (auto* an_expr = dynamic_cast<ArithmeticExpressionNode*>(current_node)) {
            TokenType left_type = getTypeRecursive(an_expr->left.get());
            if (an_expr->right) { 
                TokenType right_type = getTypeRecursive(an_expr->right.get());
                if ((left_type == TokenType::INTEGER_CONST || left_type == TokenType::REAL_CONST || left_type == TokenType::CHAR) &&
                    (right_type == TokenType::INTEGER_CONST || right_type == TokenType::REAL_CONST || right_type == TokenType::CHAR)) {
                    // Simple promotion: if either is REAL, result is REAL.
                    if (left_type == TokenType::REAL_CONST || right_type == TokenType::REAL_CONST) return TokenType::REAL_CONST;
                    // If either is INTEGER (and no REAL), result is INTEGER.
                    if (left_type == TokenType::INTEGER_CONST || right_type == TokenType::INTEGER_CONST) return TokenType::INTEGER_CONST;
                    // If both are CHAR (and no REAL/INTEGER), result is CHAR (or INT depending on lang spec)
                    if (left_type == TokenType::CHAR && right_type == TokenType::CHAR) return TokenType::CHAR; // Or INTEGER_CONST for char arithmetic
                }
                error_handler_.report_semantic_error(an_expr->op, "Type mismatch or unsupported operation in arithmetic expression. Left: " + Token(left_type,"",0,0).toString() + ", Right: " + Token(right_type,"",0,0).toString());
                throw std::runtime_error("Type mismatch or unsupported operation in arithmetic expression.");
            }
            return left_type; 
        } else if (auto* term = dynamic_cast<TermNode*>(current_node)) {
            TokenType left_type = getTypeRecursive(term->left.get());
            if (term->right) { 
                TokenType right_type = getTypeRecursive(term->right.get());
                 if ((left_type == TokenType::INTEGER_CONST || left_type == TokenType::REAL_CONST || left_type == TokenType::CHAR) &&
                    (right_type == TokenType::INTEGER_CONST || right_type == TokenType::REAL_CONST || right_type == TokenType::CHAR)) {
                    if (left_type == TokenType::REAL_CONST || right_type == TokenType::REAL_CONST) return TokenType::REAL_CONST;
                    if (left_type == TokenType::INTEGER_CONST || right_type == TokenType::INTEGER_CONST) return TokenType::INTEGER_CONST;
                    if (left_type == TokenType::CHAR && right_type == TokenType::CHAR) return TokenType::CHAR; 
                }
                error_handler_.report_semantic_error(term->op, "Type mismatch or unsupported operation in term. Left: " + Token(left_type,"",0,0).toString() + ", Right: " + Token(right_type,"",0,0).toString());
                throw std::runtime_error("Type mismatch or unsupported operation in term.");
            }
            return left_type; 
        } else if (auto* factor = dynamic_cast<FactorNode*>(current_node)) {
            return getTypeRecursive(factor->content.get()); 
        } else if (auto* var = dynamic_cast<VariableNode*>(current_node)) {
            auto symbol = symbol_table_.lookup(var->identifier.lexeme);
            if (!symbol) {
                 error_handler_.report_semantic_error(var->identifier, "Undeclared variable '" + var->identifier.lexeme + "' in getTypeRecursive.");
                 throw std::runtime_error("Undeclared variable '" + var->identifier.lexeme + "' in getTypeRecursive.");
            }
            if (symbol->data_type == TokenType::INTEGER) return TokenType::INTEGER_CONST;
            if (symbol->data_type == TokenType::REAL) return TokenType::REAL_CONST;
            if (symbol->data_type == TokenType::CHAR) return TokenType::CHAR; 
            error_handler_.report_semantic_error(var->identifier, "Unknown data type for variable '" + var->identifier.lexeme + "'.");
            throw std::runtime_error("Unknown data type for variable '" + var->identifier.lexeme + "'.");
        } else if (auto* cnst = dynamic_cast<ConstantNode*>(current_node)) {
            return cnst->constant_token.type; 
        } else {
            error_handler_.report(Error::Type::GENERAL, "Unknown ASTNode type in get_expression_type.");
            throw std::runtime_error("Unknown ASTNode type in get_expression_type.");
        }
    };

    return getTypeRecursive(expr_node);
} 