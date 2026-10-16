#ifndef LEXER_H
#define LEXER_H

#include "types.h"

class Lexer {
private:
    const char* keywords[20];

    bool isKeyword(const char word[]);
    void addToken(TokenList& list, Metrics& metrics,
                  const char value[], const char type[]);

public:
    Lexer();

    bool readFile(const char filename[], char code[], int maxSize);

    void tokenize(const char code[], TokenList& tokens, Metrics& metrics);

    void printLexerReport(const char filename[],
                          const TokenList& tokens,
                          const Metrics& metrics);
};

#endif