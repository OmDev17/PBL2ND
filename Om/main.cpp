#include <iostream>
#include "Lexer.h"

using namespace std;

int main() {

    Lexer lexer;

    char code[10000];

    TokenList tokens;
    Metrics metrics;

    const char filename[] = "bubble_sort.txt";

    if (!lexer.readFile(filename, code, 10000)) {
        cout << "Error opening file!\n";
        return 1;
    }

    lexer.tokenize(code, tokens, metrics);

    lexer.printLexerReport(filename, tokens, metrics);

    return 0;
}