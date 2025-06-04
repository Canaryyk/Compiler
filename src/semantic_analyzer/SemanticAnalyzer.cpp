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
    
    TokenType var_decl_type = symbol->data_type; // 变量声明的类型: INTEGER, REAL, CHAR
    TokenType expr_eval_type = get_expression_type(node->expression.get()); // 表达式求值后的类型: INTEGER_CONST, REAL_CONST, CHAR

    bool compatible = false;
    if (var_decl_type == TokenType::INTEGER) {
        if (expr_eval_type == TokenType::INTEGER_CONST || expr_eval_type == TokenType::CHAR) {
            compatible = true; // INT = INT_CONST or INT = CHAR (char 提升为 int)
        }
    } else if (var_decl_type == TokenType::REAL) {
        if (expr_eval_type == TokenType::REAL_CONST || 
            expr_eval_type == TokenType::INTEGER_CONST || 
            expr_eval_type == TokenType::CHAR) {
            compatible = true; // REAL = REAL_CONST or REAL = INT_CONST or REAL = CHAR (char 提升后变为 int, 再提升为 real)
        }
    } else if (var_decl_type == TokenType::CHAR) {
        if (expr_eval_type == TokenType::CHAR) {
            compatible = true; // CHAR = CHAR
        }
        // 注意: CHAR = INTEGER_CONST (例如 c := 65) 在这里未被允许，如需允许，需添加规则并考虑范围。目前保持严格。
    }

    if (!compatible) {
        // 使用一个辅助函数将TokenType枚举转换为用户友好的字符串表示
        auto type_to_string = [](TokenType tt) -> std::string {
            if (tt == TokenType::INTEGER) return "integer";
            if (tt == TokenType::REAL) return "real";
            if (tt == TokenType::CHAR) return "char";
            if (tt == TokenType::INTEGER_CONST) return "integer_const";
            if (tt == TokenType::REAL_CONST) return "real_const";
            return "unknown_type"; // Fallback
        };
        error_handler_.report_semantic_error(node->assign_op, 
            "Type mismatch in assignment to '" + node->identifier.lexeme + 
            "'. Variable '" + node->identifier.lexeme + "' is type '" + type_to_string(var_decl_type) + 
            "' but assigned expression is type '" + type_to_string(expr_eval_type) + "'.");
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
            TokenType left_actual_type = getTypeRecursive(an_expr->left.get());
            if (an_expr->right) { // 二元运算
                TokenType right_actual_type = getTypeRecursive(an_expr->right.get());
                Token op_token = an_expr->op;

                // 规则：CHAR类型在算术运算中提升为INTEGER_CONST
                TokenType eval_left_type = (left_actual_type == TokenType::CHAR) ? TokenType::INTEGER_CONST : left_actual_type;
                TokenType eval_right_type = (right_actual_type == TokenType::CHAR) ? TokenType::INTEGER_CONST : right_actual_type;

                // 规则：不允许CHAR类型直接参与乘法或除法 (即使提升后)
                // (或者，如果语言规范允许 char*int，则此检查可以调整)
                if (op_token.type == TokenType::MULTIPLY || op_token.type == TokenType::DIVIDE) {
                    if (left_actual_type == TokenType::CHAR || right_actual_type == TokenType::CHAR) {
                        error_handler_.report_semantic_error(op_token, "CHAR type cannot be directly used with '" + op_token.lexeme + "' operations.");
                        throw std::runtime_error("Invalid operation with CHAR type for * or /.");
                    }
                }

                // 核心算术类型检查 (基于提升后的类型)
                if ((eval_left_type == TokenType::INTEGER_CONST || eval_left_type == TokenType::REAL_CONST) &&
                    (eval_right_type == TokenType::INTEGER_CONST || eval_right_type == TokenType::REAL_CONST)) {
                    // 提升规则：REAL 优先
                    if (eval_left_type == TokenType::REAL_CONST || eval_right_type == TokenType::REAL_CONST) {
                        return TokenType::REAL_CONST;
                    }
                    return TokenType::INTEGER_CONST; // 结果为整数 (可能来自 INT op INT, 或 (promoted CHAR) op INT等)
                }
                error_handler_.report_semantic_error(op_token, "Type mismatch for operator '" + op_token.lexeme + 
                                                     "'. Operands are '" + Token(left_actual_type,"",0,0).toString() + 
                                                     "' and '" + Token(right_actual_type,"",0,0).toString() + ".");
                throw std::runtime_error("Type mismatch or unsupported operation in arithmetic expression.");
            }
            return left_actual_type; // 单一操作数 (例如, <项> 本身)
        } else if (auto* term = dynamic_cast<TermNode*>(current_node)) {
            TokenType left_actual_type = getTypeRecursive(term->left.get());
            if (term->right) { // 二元运算
                TokenType right_actual_type = getTypeRecursive(term->right.get());
                Token op_token = term->op;

                TokenType eval_left_type = (left_actual_type == TokenType::CHAR) ? TokenType::INTEGER_CONST : left_actual_type;
                TokenType eval_right_type = (right_actual_type == TokenType::CHAR) ? TokenType::INTEGER_CONST : right_actual_type;

                if (op_token.type == TokenType::MULTIPLY || op_token.type == TokenType::DIVIDE) {
                    if (left_actual_type == TokenType::CHAR || right_actual_type == TokenType::CHAR) {
                        error_handler_.report_semantic_error(op_token, "CHAR type cannot be directly used with '" + op_token.lexeme + "' operations.");
                        throw std::runtime_error("Invalid operation with CHAR type for * or /.");
                    }
                }
                
                if ((eval_left_type == TokenType::INTEGER_CONST || eval_left_type == TokenType::REAL_CONST) &&
                    (eval_right_type == TokenType::INTEGER_CONST || eval_right_type == TokenType::REAL_CONST)) {
                    if (eval_left_type == TokenType::REAL_CONST || eval_right_type == TokenType::REAL_CONST) {
                        return TokenType::REAL_CONST;
                    }
                    return TokenType::INTEGER_CONST;
                }
                error_handler_.report_semantic_error(op_token, "Type mismatch for operator '" + op_token.lexeme + 
                                                     "'. Operands are '" + Token(left_actual_type,"",0,0).toString() + 
                                                     "' and '" + Token(right_actual_type,"",0,0).toString() + ".");
                throw std::runtime_error("Type mismatch or unsupported operation in term.");
            }
            return left_actual_type; // 单一操作数 (例如, <因子> 本身)
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