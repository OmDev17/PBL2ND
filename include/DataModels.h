#ifndef DATA_MODELS_H
#define DATA_MODELS_H

#include <string>
#include <ctime>

// Shared constants (fixed arrays only, no STL containers).
const int MAX_TOKENS = 3000;
const int MAX_TOKEN_LEN = 64;
const int MAX_LINE_LEN = 1024;
const int MAX_CHILDREN = 20;
const int MAX_FUNCTIONS = 50;
const int MAX_NAME_LEN = 64;
const int MAX_COMPLEXITY_LEN = 32;
const int MAX_PATH_LEN = 260;
const int MAX_PROJECT_LEN = 100;
const int MAX_RECORDS = 500;
const int MAX_SOURCE_LEN = 8000;

// Token categories produced by the universal lexer.
enum TokenType {
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    DELIMITER,
    NEWLINE
};

// One lexical token with position/indent info.
struct Token {
    std::string value;
    TokenType type;
    int line;
    int indentLevel;
};

// Raw-array token list wrapper.
struct TokenList {
    Token tokens[MAX_TOKENS];
    int count;
    TokenList() : count(0) {}
};

// Loop bound classification for N-ary loop tree nodes.
enum LoopBound {
    CONSTANT,
    LOGARITHMIC,
    LINEAR
};

// N-ary tree node representing a loop scope.
struct LoopNode {
    LoopBound bound;
    LoopNode* children[20];
    int childCount;
    LoopNode() : bound(LINEAR), childCount(0) {
        for (int i = 0; i < 20; i++) children[i] = 0;
    }
};

// Unified analysis result (time complexity only).
struct ComplexityResult {
    std::string bigO;
    int priorityRank;
    int loopCount;
    int maxLoopDepth;
    bool hasDirectRecursion;
    bool hasIndirectRecursion;
    std::string recursionType;
    ComplexityResult()
        : bigO("O(1)"), priorityRank(1), loopCount(0), maxLoopDepth(0),
          hasDirectRecursion(false), hasIndirectRecursion(false),
          recursionType("None") {}
};

// Stored submission record.
struct SubmissionRecord {
    char projectName[MAX_PROJECT_LEN];
    char timestamp[32];
    char bigO[MAX_COMPLEXITY_LEN];
    int priorityRank;
    int loopDepth;
    char recursionType[MAX_NAME_LEN];
    char sourceCode[MAX_SOURCE_LEN];
    SubmissionRecord() : priorityRank(1), loopDepth(0) {
        projectName[0] = '\0';
        timestamp[0] = '\0';
        bigO[0] = '\0';
        recursionType[0] = '\0';
        sourceCode[0] = '\0';
    }
};

#endif // DATA_MODELS_H
