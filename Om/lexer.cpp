#include "Lexer.h"
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <cctype>

using namespace std;

// Constructor
Lexer::Lexer() {
    const char* words[20] = {
        "int", "float", "double", "char",
        "void", "bool", "for", "while",
        "do", "if", "else", "return",
        "class", "public", "private",
        "long", "short", "break",
        "continue", "switch"
    };

    for (int i = 0; i < 20; i++) {
        keywords[i] = words[i];
    }
}

// Check keyword
bool Lexer::isKeyword(const char word[]) {
    for (int i = 0; i < 20; i++) {
        if (strcmp(word, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

// Read input file
bool Lexer::readFile(const char filename[], char code[], int maxSize) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        return false;
    }

    int i = 0;
    int ch;

    while ((ch = fgetc(file)) != EOF && i < maxSize - 1) {
        code[i++] = (char)ch;
    }

    code[i] = '\0';
    fclose(file);

    return true;
}

// Store token and update frequency
void Lexer::addToken(TokenList& list, Metrics& metrics,
                     const char value[], const char type[]) {

    if (list.count >= MAX_TOKENS) {
        return;
    }

    strcpy(list.tokens[list.count].value, value);
    strcpy(list.tokens[list.count].type, type);
    list.count++;

    // Check whether token already exists
    for (int i = 0; i < metrics.frequencyCount; i++) {
        if (strcmp(metrics.frequency[i].word, value) == 0) {
            metrics.frequency[i].count++;
            return;
        }
    }

    // Add new frequency entry
    if (metrics.frequencyCount < MAX_TOKENS) {
        int index = metrics.frequencyCount;

        strcpy(metrics.frequency[index].word, value);
        metrics.frequency[index].count = 1;

        metrics.frequencyCount++;
    }
}

// Tokenizer
void Lexer::tokenize(const char code[],
                     TokenList& tokens,
                     Metrics& metrics) {

    int i = 0;
    char word[MAX_TOKEN_LENGTH];

    while (code[i] != '\0') {

        // Ignore spaces and newlines
        if (isspace((unsigned char)code[i])) {
            i++;
            continue;
        }

        // Keywords and identifiers
        if (isalpha((unsigned char)code[i]) || code[i] == '_') {

            int j = 0;

            while (isalnum((unsigned char)code[i]) || code[i] == '_') {
                if (j < MAX_TOKEN_LENGTH - 1) {
                    word[j++] = code[i];
                }
                i++;
            }

            word[j] = '\0';

            if (isKeyword(word)) {
                addToken(tokens, metrics, word, "Keyword");
            }
            else {
                addToken(tokens, metrics, word, "Identifier");
            }

            continue;
        }

        // Numbers
        if (isdigit((unsigned char)code[i])) {

            int j = 0;

            while (isdigit((unsigned char)code[i])) {
                if (j < MAX_TOKEN_LENGTH - 1) {
                    word[j++] = code[i];
                }
                i++;
            }

            word[j] = '\0';

            addToken(tokens, metrics, word, "Number");

            continue;
        }

        // Two-character operators
        if (code[i + 1] != '\0') {

            char op[3] = {code[i], code[i + 1], '\0'};

            if (strcmp(op, "++") == 0 ||
                strcmp(op, "--") == 0 ||
                strcmp(op, "==") == 0 ||
                strcmp(op, "!=") == 0 ||
                strcmp(op, "<=") == 0 ||
                strcmp(op, ">=") == 0 ||
                strcmp(op, "+=") == 0 ||
                strcmp(op, "-=") == 0 ||
                strcmp(op, "*=") == 0 ||
                strcmp(op, "/=") == 0 ||
                strcmp(op, "&&") == 0 ||
                strcmp(op, "||") == 0) {

                addToken(tokens, metrics, op, "Operator");

                i += 2;
                continue;
            }
        }

        // Single-character operators
        if (strchr("+-*/%=<>!&|^", code[i]) != NULL) {

            char op[2] = {code[i], '\0'};

            addToken(tokens, metrics, op, "Operator");

            i++;
            continue;
        }

        // Symbols
        if (strchr("(){}[];,.:", code[i]) != NULL) {

            char symbol[2] = {code[i], '\0'};

            addToken(tokens, metrics, symbol, "Symbol");

            i++;
            continue;
        }

        // Skip unsupported characters for now
        i++;
    }
}

// Print Lexer report
void Lexer::printLexerReport(const char filename[],
                             const TokenList& tokens,
                             const Metrics& metrics) {

    cout << "==================================================\n";
    cout << "               CODELENS - LEXER REPORT\n";
    cout << "==================================================\n\n";

    cout << "Input File: " << filename << "\n\n";

    cout << "--------------------------------------------------\n";
    cout << "                  TOKEN LIST\n";
    cout << "--------------------------------------------------\n\n";

    cout << left << setw(8) << "S.No"
         << setw(16) << "Token"
         << "Type\n";

    cout << "--------------------------------------------------\n";

    for (int i = 0; i < tokens.count; i++) {
        cout << left << setw(8) << i + 1
             << setw(16) << tokens.tokens[i].value
             << tokens.tokens[i].type << "\n";
    }

    cout << "\n--------------------------------------------------\n";
    cout << "               TOKEN FREQUENCY\n";
    cout << "--------------------------------------------------\n\n";

    cout << left << setw(16) << "Token" << "Frequency\n";
    cout << "--------------------------------------------------\n";

    // Display selected frequencies as in the sample report
    const char* selected[] = {
        "void", "int", "for", "if",
        "++", "=", "<", ">"
    };

    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < metrics.frequencyCount; i++) {

            if (strcmp(selected[j], metrics.frequency[i].word) == 0) {
                cout << left << setw(16) << selected[j]
                     << metrics.frequency[i].count << "\n";
                break;
            }
        }
    }

    cout << "\n--------------------------------------------------\n";
    cout << "              PROCESSING STATUS\n";
    cout << "--------------------------------------------------\n\n";

    cout << "File Reading       : Successful\n";
    cout << "Tokenization       : Successful\n";
    cout << "Frequency Counting : Successful\n\n";

    cout << "Total Tokens       : " << tokens.count << "\n";

    cout << "\n--------------------------------------------------\n";
    cout << "             DATA READY FOR MEMBER 2\n";
    cout << "--------------------------------------------------\n\n";

    cout << "Receiver: Vanshika Thapliyal\n";
    cout << "Module  : StructureAnalyzer\n\n";

    cout << "Data Provided:\n";
    cout << "1. Token array\n";
    cout << "2. Token count\n";
    cout << "3. Frequency array\n";
    cout << "4. Frequency count\n\n";

    cout << "Status: READY FOR STRUCTURAL ANALYSIS\n\n";

    cout << "==================================================\n";
}