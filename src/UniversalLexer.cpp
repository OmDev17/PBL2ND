#include "../include/UniversalLexer.h"
#include <fstream>
#include <cctype>

// Seed keyword hash table for universal control/function keywords.
UniversalLexer::UniversalLexer() {
    for (int i = 0; i < HASH_SIZE; i++) table[i].used = false;
    const char* kws[] = {
        "for","while","loop","repeat","foreach","if","elif","else",
        "switch","def","function","fn","func","void","int","return", 0
    };
    for (int i = 0; kws[i] != 0; i++) insertKeyword(kws[i]);
}

unsigned int UniversalLexer::hashKey(const char* s) const {
    unsigned int h = 5381;
    for (int i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        h = ((h << 5) + h) + (unsigned char)c;
    }
    return h % HASH_SIZE;
}

void UniversalLexer::toLowerCopy(const char* src, char* dest, int maxLen) const {
    int i = 0;
    for (; src[i] != '\0' && i < maxLen - 1; i++) {
        char c = src[i];
        dest[i] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
    }
    dest[i] = '\0';
}

void UniversalLexer::insertKeyword(const char* word) {
    unsigned int idx = hashKey(word);
    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int pos = (idx + i) % HASH_SIZE;
        if (!table[pos].used) {
            int j = 0;
            while (word[j] != '\0' && j < MAX_TOKEN_LEN - 1) {
                table[pos].key[j] = word[j]; j++;
            }
            table[pos].key[j] = '\0';
            table[pos].used = true;
            return;
        }
    }
}

bool UniversalLexer::isKeyword(const char* word) const {
    char lower[MAX_TOKEN_LEN];
    toLowerCopy(word, lower, MAX_TOKEN_LEN);
    unsigned int idx = hashKey(lower);
    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int pos = (idx + i) % HASH_SIZE;
        if (!table[pos].used) return false;
        bool match = true;
        for (int j = 0; ; j++) {
            if (table[pos].key[j] != lower[j]) { match = false; break; }
            if (lower[j] == '\0') break;
        }
        if (match) return true;
    }
    return false;
}

bool UniversalLexer::isAlphaChar(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool UniversalLexer::isDigitChar(char c) const { return c >= '0' && c <= '9'; }
bool UniversalLexer::isIdentStart(char c) const { return isAlphaChar(c); }
bool UniversalLexer::isIdentChar(char c) const { return isAlphaChar(c) || isDigitChar(c); }
bool UniversalLexer::isOperatorChar(char c) const {
    return c=='+'||c=='-'||c=='*'||c=='/'||c=='%'||c=='='||c=='!'||
           c=='<'||c=='>'||c=='&'||c=='|'||c=='^'||c=='~'||c==':';
}
bool UniversalLexer::isDelimiterChar(char c) const {
    return c=='('||c==')'||c=='{'||c=='}'||c=='['||c==']'||
           c==','||c==';'||c=='.';
}

void UniversalLexer::addToken(TokenList& list, const std::string& value,
                              TokenType type, int line, int indent) {
    if (list.count >= MAX_TOKENS) return;
    list.tokens[list.count].value = value;
    list.tokens[list.count].type = type;
    list.tokens[list.count].line = line;
    list.tokens[list.count].indentLevel = indent;
    list.count++;
}

// Returns true if line starts with #include or #define (keep for C/C++).
bool UniversalLexer::isIncludeDefineLine(const char* lineStart) const {
    int i = 0;
    while (lineStart[i] == ' ' || lineStart[i] == '\t') i++;
    if (lineStart[i] != '#') return false;
    i++;
    while (lineStart[i] == ' ' || lineStart[i] == '\t') i++;
    const char* inc = "include";
    const char* def = "define";
    bool isInc = true, isDef = true;
    for (int j = 0; inc[j] != '\0'; j++) { if (lineStart[i + j] != inc[j]) { isInc = false; break; } }
    for (int j = 0; def[j] != '\0'; j++) { if (lineStart[i + j] != def[j]) { isDef = false; break; } }
    return isInc || isDef;
}

bool UniversalLexer::readFile(const char* filename, char* buffer, int maxSize) {
    std::ifstream in(filename, std::ios::binary);
    if (!in.is_open()) return false;
    int i = 0; char c;
    while (in.get(c) && i < maxSize - 1) buffer[i++] = c;
    buffer[i] = '\0';
    in.close();
    return true;
}

// Tokenize with line/indent tracking and brace/delimiter awareness.
void UniversalLexer::tokenize(const char* code, TokenList& tokens) {
    tokens.count = 0;
    int i = 0, lineNum = 1;
    while (code[i] != '\0') {
        // Compute indentation at start of line.
        int indent = 0, j = i;
        while (code[j] == ' ' || code[j] == '\t') {
            indent += (code[j] == '\t') ? 4 : 1; j++;
        }
        bool atLineStart = true;
        // Process characters on this line.
        while (code[i] != '\0' && code[i] != '\n') {
            char c = code[i];
            if (c == ' ' || c == '\t' || c == '\r') { i++; continue; }
            // Python-style # comments (but keep #include/#define).
            if (c == '#') {
                if (atLineStart && isIncludeDefineLine(&code[i])) {
                    // Tokenize the directive line normally.
                    addToken(tokens, "#", OPERATOR, lineNum, indent);
                    i++;
                    continue;
                }
                while (code[i] != '\0' && code[i] != '\n') i++;
                break;
            }
            atLineStart = false;
            // C++/Java/JS // line comment.
            if (c == '/' && code[i + 1] == '/') {
                while (code[i] != '\0' && code[i] != '\n') i++;
                break;
            }
            // Block comment /* */.
            if (c == '/' && code[i + 1] == '*') {
                i += 2;
                while (code[i] != '\0' && !(code[i] == '*' && code[i + 1] == '/')) {
                    if (code[i] == '\n') { addToken(tokens, "\n", NEWLINE, lineNum, 0); lineNum++; }
                    i++;
                }
                if (code[i] != '\0') i += 2;
                continue;
            }
            // String/char literals (kept as single NUMBER-like token for simplicity).
            if (c == '"' || c == '\'') {
                char q = c; std::string lit; lit += c; i++;
                while (code[i] != '\0' && code[i] != q && code[i] != '\n') {
                    if (code[i] == '\\' && code[i + 1] != '\0') { lit += code[i++]; }
                    if (code[i] != '\0') lit += code[i++];
                }
                if (code[i] == q) { lit += q; i++; }
                addToken(tokens, lit, NUMBER, lineNum, indent);
                continue;
            }
            // Identifier or keyword.
            if (isIdentStart(c)) {
                std::string word;
                while (isIdentChar(code[i])) { word += code[i++]; }
                TokenType t = isKeyword(word.c_str()) ? KEYWORD : IDENTIFIER;
                addToken(tokens, word, t, lineNum, indent);
                continue;
            }
            // Number.
            if (isDigitChar(c)) {
                std::string num;
                while (isDigitChar(code[i]) || code[i] == '.') { num += code[i++]; }
                addToken(tokens, num, NUMBER, lineNum, indent);
                continue;
            }
            // Two-char operators.
            char n = code[i + 1];
            if ((c=='*'&&n=='=')||(c=='/'&&n=='=')||(c=='>'&&n=='>')||(c=='<'&&n=='<')||
                (c=='='&&n=='=')||(c=='!'&&n=='=')||(c=='<'&&n=='=')||(c=='>'&&n=='=')||
                (c=='+'&&n=='+')||(c=='-'&&n=='-')||(c=='+'&&n=='=')||(c=='-'&&n=='=')||
                (c=='&'&&n=='&')||(c=='|'&&n=='|')||(c=='-'&&n=='>')) {
                std::string op; op += c; op += n;
                addToken(tokens, op, OPERATOR, lineNum, indent);
                i += 2; continue;
            }
            // Single-char delimiter/operator.
            if (isDelimiterChar(c)) {
                std::string d; d += c;
                addToken(tokens, d, DELIMITER, lineNum, indent);
                i++; continue;
            }
            if (isOperatorChar(c) || c == '?' || c == '\\') {
                std::string op; op += c;
                addToken(tokens, op, OPERATOR, lineNum, indent);
                i++; continue;
            }
            // Fallback: skip unknown char.
            i++;
        }
        // Newline token for indentation-based structure.
        if (code[i] == '\n') {
            addToken(tokens, "\n", NEWLINE, lineNum, 0);
            i++; lineNum++;
        }
    }
}
