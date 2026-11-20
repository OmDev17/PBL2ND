#ifndef RECURSION_GRAPH_H
#define RECURSION_GRAPH_H

#include "DataModels.h"

// Function node in the custom directed call graph.
struct FunctionNode {
    std::string name;
    int selfCallCount;
    bool halvesInput;
    FunctionNode() : name(""), selfCallCount(0), halvesInput(false) {}
};

// Builds call graph and detects recursion/complexity via DFS.
class RecursionGraph {
private:
    FunctionNode functions[MAX_FUNCTIONS];
    int adjMatrix[MAX_FUNCTIONS][MAX_FUNCTIONS];
    int funcCount;

    bool equalsIgnoreCase(const std::string& a, const char* b) const;
    int findFunction(const std::string& name) const;
    int addFunction(const std::string& name);
    int findBlockEndBraces(const TokenList& tokens, int openBrace) const;
    int findBlockEndIndent(const TokenList& tokens, int start, int baseIndent) const;
    void detectDefinitions(const TokenList& tokens);
    void detectCalls(const TokenList& tokens);
    bool dfsCycle(int u, bool visited[], bool recStack[], bool& indirect) const;
    void analyzeRecursion(ComplexityResult& result) const;

public:
    RecursionGraph();
    void reset();
    void analyze(const TokenList& tokens, ComplexityResult& result);
};

#endif // RECURSION_GRAPH_H
