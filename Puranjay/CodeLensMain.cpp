#include <stdio.h>
#include <string.h>

#include "../Om/lexer.h"
#include "../Vanshika/structure.h"
#include "../Asmit/ComplexityModel.h"
#include "../Asmit/ComplexityEngine.h"
#include "filemanager.h"
#include "prioritymanager.h"

#define CODE_BUFFER_SIZE 10000
#define MAX_PATH_SIZE 260
#define TEMP_FILENAME "temp.txt"

static Node nodes[MAX_TOKENS];
static int loopTokenIndexes[MAX_TOKENS];
static int loopBodyStarts[MAX_TOKENS];
static int loopBodyEnds[MAX_TOKENS];
static int loopParents[MAX_TOKENS];

static void printReport(const char filename[], const Result& result,
                        const Metrics& metrics, int tokenCount);

static bool readInputLine(char value[], int size) {
    if (fgets(value, size, stdin) == NULL) {
        return false;
    }

    int length = (int)strlen(value);
    while (length > 0 && (value[length - 1] == '\n' || value[length - 1] == '\r')) {
        value[--length] = '\0';
    }
    return true;
}

static void cleanSource(char code[]) {
    enum State {
        NORMAL,
        LINE_COMMENT,
        BLOCK_COMMENT,
        STRING_LITERAL,
        CHARACTER_LITERAL,
        PREPROCESSOR
    };

    State state = NORMAL;
    bool lineWhitespace = true;

    for (int i = 0; code[i] != '\0'; i++) {
        char current = code[i];

        if (state == LINE_COMMENT || state == PREPROCESSOR) {
            if (current == '\n') {
                state = NORMAL;
                lineWhitespace = true;
            } else {
                code[i] = ' ';
            }
            continue;
        }

        if (state == BLOCK_COMMENT) {
            if (current == '*' && code[i + 1] == '/') {
                code[i] = ' ';
                code[++i] = ' ';
                state = NORMAL;
            } else if (current == '\n') {
                lineWhitespace = true;
            } else {
                code[i] = ' ';
            }
            continue;
        }

        if (state == STRING_LITERAL || state == CHARACTER_LITERAL) {
            char terminator = state == STRING_LITERAL ? '"' : '\'';
            if (current == '\\' && code[i + 1] != '\0' && code[i + 1] != '\n') {
                code[i] = ' ';
                code[++i] = ' ';
            } else if (current == terminator) {
                code[i] = ' ';
                state = NORMAL;
            } else if (current != '\n') {
                code[i] = ' ';
            } else {
                lineWhitespace = true;
            }
            continue;
        }

        if (current == '\n') {
            lineWhitespace = true;
            continue;
        }

        if (lineWhitespace && (current == ' ' || current == '\t' || current == '\r')) {
            continue;
        }

        if (lineWhitespace && current == '#') {
            code[i] = ' ';
            state = PREPROCESSOR;
            continue;
        }

        if (current != ' ' && current != '\t' && current != '\r') {
            lineWhitespace = false;
        }

        if (current == '/' && code[i + 1] == '/') {
            code[i] = ' ';
            code[++i] = ' ';
            state = LINE_COMMENT;
        } else if (current == '/' && code[i + 1] == '*') {
            code[i] = ' ';
            code[++i] = ' ';
            state = BLOCK_COMMENT;
        } else if (current == '"') {
            code[i] = ' ';
            state = STRING_LITERAL;
        } else if (current == '\'') {
            code[i] = ' ';
            state = CHARACTER_LITERAL;
        }
    }
}

static bool tokenIs(const Token& token, const char value[]) {
    return strcmp(token.value, value) == 0;
}

static int findMatchingParen(const TokenList& tokens, int openIndex) {
    int depth = 0;

    for (int i = openIndex; i < tokens.count; i++) {
        if (tokenIs(tokens.tokens[i], "(")) {
            depth++;
        } else if (tokenIs(tokens.tokens[i], ")")) {
            depth--;
            if (depth == 0) {
                return i;
            }
        }
    }

    return -1;
}

static bool findLoopBody(const TokenList& tokens, int loopIndex,
                         int& bodyStart, int& bodyEnd) {
    int openParen = loopIndex + 1;
    while (openParen < tokens.count && !tokenIs(tokens.tokens[openParen], "(")) {
        openParen++;
    }

    if (openParen == tokens.count) {
        return false;
    }

    int closeParen = findMatchingParen(tokens, openParen);
    if (closeParen < 0 || closeParen + 1 >= tokens.count) {
        return false;
    }

    int first = closeParen + 1;
    if (tokenIs(tokens.tokens[first], "{")) {
        int depth = 0;
        for (int i = first; i < tokens.count; i++) {
            if (tokenIs(tokens.tokens[i], "{")) {
                depth++;
            } else if (tokenIs(tokens.tokens[i], "}")) {
                depth--;
                if (depth == 0) {
                    bodyStart = first + 1;
                    bodyEnd = i;
                    return true;
                }
            }
        }
        return false;
    }

    for (int i = first; i < tokens.count; i++) {
        if (tokenIs(tokens.tokens[i], ";")) {
            bodyStart = first;
            bodyEnd = i + 1;
            return true;
        }
    }

    bodyStart = first;
    bodyEnd = tokens.count;
    return true;
}

static void appendNode(Node*& first, Node* node) {
    if (first == NULL) {
        first = node;
        return;
    }

    Node* last = first;
    while (last->nextSibling != NULL) {
        last = last->nextSibling;
    }
    last->nextSibling = node;
}

static void buildLoopTree(const TokenList& tokens, Node& root) {
    int loopCount = 0;
    root.type = SIMPLE;
    root.bound = CONSTANT_BOUND;
    root.firstChild = NULL;
    root.elseChild = NULL;
    root.nextSibling = NULL;

    for (int i = 0; i < tokens.count && loopCount < MAX_TOKENS; i++) {
        if (strcmp(tokens.tokens[i].type, "Keyword") == 0 &&
            (tokenIs(tokens.tokens[i], "for") || tokenIs(tokens.tokens[i], "while"))) {
            int bodyStart;
            int bodyEnd;
            if (findLoopBody(tokens, i, bodyStart, bodyEnd)) {
                loopTokenIndexes[loopCount] = i;
                loopBodyStarts[loopCount] = bodyStart;
                loopBodyEnds[loopCount] = bodyEnd;
                loopParents[loopCount] = -1;

                nodes[loopCount].type = LOOP;
                nodes[loopCount].bound = N_BOUND;
                nodes[loopCount].firstChild = NULL;
                nodes[loopCount].elseChild = NULL;
                nodes[loopCount].nextSibling = NULL;
                loopCount++;
            }
        }
    }

    for (int child = 0; child < loopCount; child++) {
        int smallestBody = MAX_TOKENS + 1;
        for (int parent = 0; parent < loopCount; parent++) {
            int bodySize = loopBodyEnds[parent] - loopBodyStarts[parent];
            if (parent != child &&
                loopBodyStarts[parent] <= loopTokenIndexes[child] &&
                loopTokenIndexes[child] < loopBodyEnds[parent] &&
                bodySize < smallestBody) {
                smallestBody = bodySize;
                loopParents[child] = parent;
            }
        }
    }

    for (int i = 0; i < loopCount; i++) {
        if (loopParents[i] < 0) {
            appendNode(root.firstChild, &nodes[i]);
        } else {
            appendNode(nodes[loopParents[i]].firstChild, &nodes[i]);
        }
    }
}

static bool analyzeFile(const char filename[], Result& result, Metrics& metrics,
                        int& tokenCount) {
    static TokenList tokens;
    static char code[CODE_BUFFER_SIZE];
    static Node root;

    tokens.count = 0;
    metrics.frequencyCount = 0;
    metrics.loopCount = 0;
    metrics.conditionalCount = 0;
    metrics.maxLoopDepth = 0;

    Lexer lexer;
    if (!lexer.readFile(filename, code, CODE_BUFFER_SIZE)) {
        printf("Cannot open source file: %s\n", filename);
        return false;
    }

    cleanSource(code);
    lexer.tokenize(code, tokens, metrics);
    tokenCount = tokens.count;

    StructureAnalyzer structureAnalyzer;
    if (!structureAnalyzer.analyze(tokens, metrics)) {
        printf("Brackets are unbalanced in: %s\n", filename);
        return false;
    }

    buildLoopTree(tokens, root);
    ComplexityEngine complexityEngine;
    result = complexityEngine.analyze(&root);
    metrics.maxLoopDepth = result.maxLoopDepth;
    return true;
}

static bool writeTokenHandoff(const TokenList& tokens) {
    FILE* file = fopen(TEMP_FILENAME, "w");
    if (file == NULL) {
        printf("Could not create %s.\n", TEMP_FILENAME);
        return false;
    }

    fprintf(file, "%d\n", tokens.count);
    for (int i = 0; i < tokens.count; i++) {
        fprintf(file, "%s\t%s\n", tokens.tokens[i].value,
                tokens.tokens[i].type);
    }
    fclose(file);
    return true;
}

static bool readTokenHandoff(TokenList& tokens) {
    FILE* file = fopen(TEMP_FILENAME, "r");
    if (file == NULL) {
        return false;
    }

    char line[MAX_TOKEN_LENGTH + 32];
    if (fgets(line, sizeof(line), file) == NULL ||
        sscanf(line, "%d", &tokens.count) != 1 ||
        tokens.count < 0 || tokens.count > MAX_TOKENS) {
        fclose(file);
        return false;
    }

    for (int i = 0; i < tokens.count; i++) {
        if (fgets(line, sizeof(line), file) == NULL ||
            sscanf(line, "%99[^\t]\t%19[^\r\n]",
                   tokens.tokens[i].value, tokens.tokens[i].type) != 2) {
            fclose(file);
            return false;
        }
    }
    fclose(file);
    return true;
}

static bool appendStructureHandoff(const Metrics& metrics) {
    FILE* file = fopen(TEMP_FILENAME, "a");
    if (file == NULL) {
        return false;
    }
    fprintf(file, "VANSHIKA %d %d %d\n", metrics.loopCount,
            metrics.conditionalCount, metrics.maxLoopDepth);
    fclose(file);
    return true;
}

static bool readStructureHandoff(Metrics& metrics) {
    FILE* file = fopen(TEMP_FILENAME, "r");
    if (file == NULL) {
        return false;
    }

    char line[128];
    bool found = false;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "VANSHIKA %d %d %d", &metrics.loopCount,
                   &metrics.conditionalCount, &metrics.maxLoopDepth) == 3) {
            found = true;
        }
    }
    fclose(file);
    return found;
}

static bool analyzeSubmittedCode(const char source[], Result& result,
                                 Metrics& metrics, int& tokenCount) {
    static TokenList tokens;
    static char workingCode[CODE_BUFFER_SIZE];
    static Node root;

    strncpy(workingCode, source, CODE_BUFFER_SIZE - 1);
    workingCode[CODE_BUFFER_SIZE - 1] = '\0';
    cleanSource(workingCode);

    tokens.count = 0;
    metrics.frequencyCount = 0;
    metrics.loopCount = 0;
    metrics.conditionalCount = 0;
    metrics.maxLoopDepth = 0;

    Lexer lexer;
    lexer.tokenize(workingCode, tokens, metrics);
    tokenCount = tokens.count;

    if (!writeTokenHandoff(tokens) || !readTokenHandoff(tokens)) {
        printf("Om's token handoff through %s failed.\n", TEMP_FILENAME);
        return false;
    }

    StructureAnalyzer structureAnalyzer;
    if (!structureAnalyzer.analyze(tokens, metrics)) {
        printf("The submitted code has unbalanced brackets.\n");
        return false;
    }
    if (!appendStructureHandoff(metrics) || !readStructureHandoff(metrics)) {
        printf("Vanshika's metrics handoff through %s failed.\n", TEMP_FILENAME);
        return false;
    }

    buildLoopTree(tokens, root);
    ComplexityEngine complexityEngine;
    result = complexityEngine.analyze(&root);
    metrics.maxLoopDepth = result.maxLoopDepth;
    strncpy(metrics.timeComplexity, result.bigO,
            sizeof(metrics.timeComplexity) - 1);
    metrics.timeComplexity[sizeof(metrics.timeComplexity) - 1] = '\0';

    FILE* file = fopen(TEMP_FILENAME, "a");
    if (file == NULL) {
        return false;
    }
    fprintf(file, "ASMIT %s degree=%d loops=%d depth=%d\n", result.bigO,
            result.degree, result.loops, result.maxLoopDepth);
    fclose(file);
    return true;
}

static bool makeTitleFilename(const char title[], char filename[], int size) {
    int out = 0;
    for (int i = 0; title[i] != '\0' && out < size - 5; i++) {
        unsigned char character = (unsigned char)title[i];
        if (isalnum(character) || character == '-' || character == '_') {
            filename[out++] = (char)character;
        } else if (character == ' ' && out > 0 && filename[out - 1] != '_') {
            filename[out++] = '_';
        }
    }
    while (out > 0 && filename[out - 1] == '_') {
        out--;
    }
    if (out == 0) {
        return false;
    }
    strcpy(filename + out, ".txt");
    return true;
}

static bool writeSubmittedSource(const char filename[], const char title[],
                                 const char source[]) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Could not create the title file: %s\n", filename);
        return false;
    }
    fprintf(file, "Title: %s\n\nCode snippet:\n", title);
    fputs(source, file);
    if (source[0] != '\0' && source[strlen(source) - 1] != '\n') {
        fputc('\n', file);
    }
    fclose(file);
    return true;
}

static void compareWithStoredExamples(const Result& result, FILE* report) {
    FileManager fileManager;
    prioritymanager priorities;
    int submittedPriority = priorities.getpriority(result.bigO);
    int bestPriority = 100;
    string bestFile;

    for (int i = 0; i < 3; i++) {
        string complexity = fileManager.readComplexity(fileManager.files[i]);
        if (complexity.empty()) {
            continue;
        }
        int priority = priorities.getpriority(complexity);
        if (priority < bestPriority) {
            bestPriority = priority;
            bestFile = fileManager.files[i];
        }
    }

    if (bestFile.empty()) {
        fprintf(report, "Puranjay comparison: no stored examples were available.\n");
    } else if (submittedPriority < bestPriority) {
        fprintf(report, "Puranjay comparison: submitted program is more efficient than %s.\n",
                bestFile.c_str());
    } else if (submittedPriority > bestPriority) {
        fprintf(report, "Puranjay comparison: %s is more efficient.\n",
                bestFile.c_str());
    } else {
        fprintf(report, "Puranjay comparison: submitted program matches the best stored example (%s).\n",
                result.bigO);
    }

    FILE* temp = fopen(TEMP_FILENAME, "a");
    if (temp != NULL) {
        fprintf(temp, "PURANJAY comparison-priority=%d best-example=%s\n",
                submittedPriority, bestFile.empty() ? "none" : bestFile.c_str());
        fclose(temp);
    }
}

static void submitProgram() {
    char title[MAX_PATH_SIZE];
    char filename[MAX_PATH_SIZE];
    static char source[CODE_BUFFER_SIZE];
    char line[1024];

    printf("Program title: ");
    if (!readInputLine(title, sizeof(title)) || title[0] == '\0') {
        printf("A non-empty title is required.\n");
        return;
    }
    if (!makeTitleFilename(title, filename, sizeof(filename))) {
        printf("The title must contain letters or numbers.\n");
        return;
    }

    printf("Enter the code snippet, then type END on its own line:\n");
    source[0] = '\0';
    size_t used = 0;
    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (strcmp(line, "END\n") == 0 || strcmp(line, "END\r\n") == 0 ||
            strcmp(line, "END") == 0) {
            break;
        }
        size_t lineLength = strlen(line);
        if (used + lineLength >= sizeof(source)) {
            printf("Code snippet exceeds the %d-byte input limit.\n",
                   CODE_BUFFER_SIZE - 1);
            return;
        }
        memcpy(source + used, line, lineLength + 1);
        used += lineLength;
    }
    if (used == 0) {
        printf("No code was entered.\n");
        return;
    }

    if (!writeSubmittedSource(filename, title, source)) {
        return;
    }

    Result result;
    Metrics metrics;
    int tokenCount;
    if (!analyzeSubmittedCode(source, result, metrics, tokenCount)) {
        FILE* report = fopen(filename, "a");
        if (report != NULL) {
            fprintf(report, "\nAnalysis failed. Check bracket balance and temp.txt.\n");
            fclose(report);
        }
        return;
    }

    FILE* report = fopen(filename, "a");
    if (report == NULL) {
        printf("Analysis completed, but results could not be saved to %s.\n", filename);
        return;
    }
    fprintf(report, "\nProcessed information:\n");
    fprintf(report, "Tokens: %d\n", tokenCount);
    fprintf(report, "Unique token values: %d\n", metrics.frequencyCount);
    fprintf(report, "Loops: %d\n", metrics.loopCount);
    fprintf(report, "Conditionals: %d\n", metrics.conditionalCount);
    fprintf(report, "Maximum loop depth: %d\n", result.maxLoopDepth);
    fprintf(report, "Time Complexity: %s\n", result.bigO);
    compareWithStoredExamples(result, report);
    fclose(report);

    printf("\nAnalysis saved to %s\n", filename);
    printf("Member handoff data is in %s\n", TEMP_FILENAME);
    printReport(filename, result, metrics, tokenCount);
}

static void printReport(const char filename[], const Result& result,
                        const Metrics& metrics, int tokenCount) {
    printf("\nFile: %s\n", filename);
    printf("Tokens: %d\n", tokenCount);
    printf("Unique token values: %d\n", metrics.frequencyCount);
    printf("Loops: %d\n", result.loops);
    printf("Conditionals: %d\n", metrics.conditionalCount);
    printf("Maximum loop depth: %d\n", result.maxLoopDepth);
    printf("Estimated time complexity: %s\n", result.bigO);
}

static void saveRecord(const char filename[], const Result& result) {
    FILE* file = fopen("codelens_records.txt", "a");
    if (file == NULL) {
        printf("Could not open codelens_records.txt for writing.\n");
        return;
    }

    fprintf(file, "%s | %s | loops=%d | depth=%d\n",
            filename, result.bigO, result.loops, result.maxLoopDepth);
    fclose(file);
}

static void analyzeOne() {
    char filename[MAX_PATH_SIZE];
    Result result;
    Metrics metrics;
    int tokenCount;

    printf("C++ source path: ");
    if (!readInputLine(filename, MAX_PATH_SIZE)) {
        return;
    }

    if (analyzeFile(filename, result, metrics, tokenCount)) {
        printReport(filename, result, metrics, tokenCount);
        saveRecord(filename, result);
        printf("Result saved to codelens_records.txt\n");
    }
}

static void compareTwo() {
    char firstFile[MAX_PATH_SIZE];
    char secondFile[MAX_PATH_SIZE];
    Result firstResult;
    Result secondResult;
    Metrics firstMetrics;
    Metrics secondMetrics;
    int firstTokenCount;
    int secondTokenCount;

    printf("First C++ source path: ");
    if (!readInputLine(firstFile, MAX_PATH_SIZE)) {
        return;
    }
    printf("Second C++ source path: ");
    if (!readInputLine(secondFile, MAX_PATH_SIZE)) {
        return;
    }

    if (!analyzeFile(firstFile, firstResult, firstMetrics, firstTokenCount) ||
        !analyzeFile(secondFile, secondResult, secondMetrics, secondTokenCount)) {
        return;
    }

    printReport(firstFile, firstResult, firstMetrics, firstTokenCount);
    printReport(secondFile, secondResult, secondMetrics, secondTokenCount);

    ComplexityEngine complexityEngine;
    int comparison = complexityEngine.compareResults(firstResult, secondResult);

    if (comparison < 0) {
        printf("\nMore efficient estimate: %s (%s)\n",
               firstFile, firstResult.bigO);
        printf("Reason: its estimated growth degree is lower.\n");
    } else if (comparison > 0) {
        printf("\nMore efficient estimate: %s (%s)\n",
               secondFile, secondResult.bigO);
        printf("Reason: its estimated growth degree is lower.\n");
    } else {
        printf("\nTie: both files have the same estimated growth and loop depth.\n");
    }

    saveRecord(firstFile, firstResult);
    saveRecord(secondFile, secondResult);
}

int main() {
    printf("CodeLens program analyzer\n");
    submitProgram();
    return 0;
}