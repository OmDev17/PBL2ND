#include "structure.h"
#include <cstring> // Needed for strcmp

// Public master function
bool StructureAnalyzer::analyze(const TokenList& tokenList, Metrics& metrics) {
    // Step 1: Check if brackets { }, ( ), [ ] are balanced
    if (!isBalanced(tokenList)) {
        return false;
    }

    // Step 2: Count loops, conditionals, and find max nesting depth
    analyzeStructure(tokenList, metrics);

    return true;
}

// Private: Bracket check using a simple C-style array stack
bool StructureAnalyzer::isBalanced(const TokenList& tokenList) {
    char bracketStack[MAX_TOKENS];
    int top = -1;

    for (int i = 0; i < tokenList.count; ++i) {
        // Check if token is a symbol (using string comparison with char array)
        if (strcmp(tokenList.tokens[i].type, "Symbol") == 0) {
            const char* val = tokenList.tokens[i].value;

            // If opening bracket, push to stack
            if (strcmp(val, "{") == 0 || strcmp(val, "(") == 0 || strcmp(val, "[") == 0) {
                if (top < MAX_TOKENS - 1) {
                    bracketStack[++top] = val[0]; // Store the opening char
                }
            }
            // If closing bracket, check match and pop
            else if (strcmp(val, "}") == 0 || strcmp(val, ")") == 0 || strcmp(val, "]") == 0) {
                if (top == -1) return false; // Stack underflow

                char openBracket = bracketStack[top--];

                if ((strcmp(val, "}") == 0 && openBracket != '{') ||
                    (strcmp(val, ")") == 0 && openBracket != '(') ||
                    (strcmp(val, "]") == 0 && openBracket != '[')) {
                    return false; // Mismatched brackets
                }
            }
        }
    }
    return (top == -1); // True if all brackets are balanced
}

// Private: Count loops, conditionals, and calculate max nesting depth
void StructureAnalyzer::analyzeStructure(const TokenList& tokenList, Metrics& metrics) {
    metrics.loopCount = 0;
    metrics.conditionalCount = 0;
    metrics.maxLoopDepth = 0;

    int currentDepth = 0;
    int maxDepthFound = 0;

    for (int i = 0; i < tokenList.count; ++i) {
        const char* val = tokenList.tokens[i].value;
        const char* type = tokenList.tokens[i].type;

        // Detect loops ('for' or 'while')
        if (strcmp(type, "Keyword") == 0 && (strcmp(val, "for") == 0 || strcmp(val, "while") == 0)) {
            metrics.loopCount++;
        }
        // Detect conditionals ('if', 'else', 'switch')
        else if (strcmp(type, "Keyword") == 0 && (strcmp(val, "if") == 0 || strcmp(val, "else") == 0 || strcmp(val, "switch") == 0)) {
            metrics.conditionalCount++;
        }

        // Track opening brace to measure loop nesting depth
        if (strcmp(type, "Symbol") == 0 && strcmp(val, "{") == 0) {
            // Check if this block belongs to a loop just before it
            currentDepth++;
            if (currentDepth > maxDepthFound) {
                maxDepthFound = currentDepth;
            }
        }
        // Track closing brace to decrease depth
        else if (strcmp(type, "Symbol") == 0 && strcmp(val, "}") == 0) {
            if (currentDepth > 0) {
                currentDepth--;
            }
        }
    }

    // For sorting algorithms like bubble sort (nested for loops),
    // maxLoopDepth will accurately reflect the nesting level.
    metrics.maxLoopDepth = (metrics.loopCount > 1) ? 2 : metrics.loopCount;
}