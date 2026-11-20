#ifndef UNIVERSAL_LEXER_H
#define UNIVERSAL_LEXER_H

#include "DataModels.h"

// Hash entry for open-addressing keyword table.
struct HashEntry {
    char key[MAX_TOKEN_LEN];
    bool used;
    HashEntry() : used(false) { key[0] = '\0'; }
};

// Language-agnostic scanner with custom hash table and indent tracking.
class UniversalLexer {
private:
    static const int HASH_SIZE = 101;
    HashEntry table[HASH_SIZE];

    unsigned int hashKey(const char* s) const;
    void insertKeyword(const char* word);
    void toLowerCopy(const char* src, char* dest, int maxLen) const;
    bool isAlphaChar(char c) const;
    bool isDigitChar(char c) const;
    bool isIdentStart(char c) const;
    bool isIdentChar(char c) const;
    bool isOperatorChar(char c) const;
    bool isDelimiterChar(char c) const;
    void addToken(TokenList& list, const std::string& value,
                  TokenType type, int line, int indent);
    bool isIncludeDefineLine(const char* lineStart) const;

public:
    UniversalLexer();
    bool isKeyword(const char* word) const;
    bool readFile(const char* filename, char* buffer, int maxSize);
    void tokenize(const char* code, TokenList& tokens);
};

#endif // UNIVERSAL_LEXER_H
