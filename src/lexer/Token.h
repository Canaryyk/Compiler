#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include "../../webview/json.hpp"
#include "../semantic_analyzer/SymbolTable.h"

enum class TokenCategory {
    KEYWORD,
    IDENTIFIER,
    CONSTANT,
    OPERATOR,
    END_OF_FILE,
    UNKNOWN
};

// 为 TokenCategory 提供字符串转换
inline std::string to_string(TokenCategory category) {
    switch (category) {
        case TokenCategory::KEYWORD: return "KEYWORD";
        case TokenCategory::IDENTIFIER: return "IDENTIFIER";
        case TokenCategory::CONSTANT: return "CONSTANT";
        case TokenCategory::OPERATOR: return "OPERATOR";
        case TokenCategory::END_OF_FILE: return "END_OF_FILE";
        case TokenCategory::UNKNOWN: return "UNKNOWN";
        default: return "INVALID_CATEGORY";
    }
}

struct Token {
    TokenCategory category;
    int index;
};

// to_json for a single Token (remains for potential internal use)
inline void to_json(nlohmann::json& j, const Token& t, const SymbolTable& table);

// Helper to format a single token into "(c, i)" string format
inline std::string format_token_to_string(const Token& t) {
    char category_char = '?';
    switch (t.category) {
        case TokenCategory::KEYWORD:    category_char = 'k'; break;
        case TokenCategory::IDENTIFIER: category_char = 'i'; break;
        case TokenCategory::CONSTANT:   category_char = 'c'; break;
        case TokenCategory::OPERATOR:   category_char = 'p'; break;
        default: return ""; // Don't include EOF or UNKNOWN in the sequence string
    }
    // Use index + 1 for 1-based indexing in display
    return "(" + std::string(1, category_char) + ", " + std::to_string(t.index + 1) + ")";
}

// JSON serialization for a vector of Tokens, tailored for the frontend
inline void to_json(nlohmann::json& j, const std::vector<Token>& tokens, const SymbolTable& table) {
    // 1. Create the formatted token sequence string
    std::string sequence_str;
    for (const auto& token : tokens) {
        std::string formatted = format_token_to_string(token);
        if (!formatted.empty()) {
            sequence_str += formatted + " ";
        }
    }

    // 2. Create JSON for the tables (keywords, operators, etc.)
    nlohmann::json tables;
    
    // Always create the keys, even if they will be empty arrays.
    // This ensures a consistent JSON structure for the frontend.
    tables["keywords"] = nlohmann::json::array();
    tables["operators"] = nlohmann::json::array();
    tables["identifiers"] = nlohmann::json::array();
    tables["constants"] = nlohmann::json::array();

    const auto& keywords = table.get_keyword_table();
    for (size_t i = 0; i < keywords.size(); ++i) {
        tables["keywords"].push_back({{"index", i + 1}, {"value", keywords[i]}});
    }
    const auto& operators = table.get_operator_table();
    for (size_t i = 0; i < operators.size(); ++i) {
        tables["operators"].push_back({{"index", i + 1}, {"value", operators[i]}});
    }
    const auto& identifiers = table.get_simple_identifier_table();
    for (size_t i = 0; i < identifiers.size(); ++i) {
        tables["identifiers"].push_back({{"index", i + 1}, {"value", identifiers[i]}});
    }
    const auto& constants = table.get_constant_table();
    for (size_t i = 0; i < constants.size(); ++i) {
        tables["constants"].push_back({{"index", i + 1}, {"value", constants[i]}});
    }

    // 3. Assemble the final JSON object
    j = {
        {"sequence", sequence_str},
        {"tables", tables}
    };
}

// Implementation of the single token to_json
inline void to_json(nlohmann::json& j, const Token& t, const SymbolTable& table) {
    j = nlohmann::json{{"category", to_string(t.category)}, {"index", t.index}};
    switch (t.category) {
        case TokenCategory::IDENTIFIER:
            if (t.index >= 0 && static_cast<size_t>(t.index) < table.get_simple_identifier_table().size()) {
                j["value"] = table.get_simple_identifier_table()[t.index];
            }
            break;
        case TokenCategory::KEYWORD:
            if (t.index >= 0 && static_cast<size_t>(t.index) < table.get_keyword_table().size()) {
                j["value"] = table.get_keyword_table()[t.index];
            }
            break;
        case TokenCategory::OPERATOR:
            if (t.index >= 0 && static_cast<size_t>(t.index) < table.get_operator_table().size()) {
                j["value"] = table.get_operator_table()[t.index];
            }
            break;
        case TokenCategory::CONSTANT:
            if (t.index >= 0 && static_cast<size_t>(t.index) < table.get_constant_table().size()) {
                j["value"] = table.get_constant_table()[t.index];
            }
            break;
        default:
            break;
    }
}

#endif // TOKEN_H 