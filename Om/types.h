#ifndef TYPES_H
#define TYPES_H

#define MAX_TOKENS 2000
#define MAX_TOKEN_LENGTH 100

struct Token {
    char value[MAX_TOKEN_LENGTH];
    char type[20];
};

struct Frequency {
    char word[MAX_TOKEN_LENGTH];
    int count;
};

struct Metrics {
    Frequency frequency[MAX_TOKENS];
    int frequencyCount = 0;

    int loopCount = 0;
    int conditionalCount = 0;
    int maxLoopDepth = 0;

    char timeComplexity[30] = "";
    char spaceComplexity[30] = "";

    int score = 0;
};

struct TokenList {
    Token tokens[MAX_TOKENS];
    int count = 0;
};

#endif