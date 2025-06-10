#ifndef TOKEN_H
#define TOKEN_H

enum class TokenCategory {
    KEYWORD,
    IDENTIFIER,
    CONSTANT,
    OPERATOR,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenCategory category;
    int index;
};

#endif // TOKEN_H 