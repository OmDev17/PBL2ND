#pragma once
#include "types.h"

class StructureAnalyzer {
private:
    bool isBalanced(const TokenList& tokenList);
    void analyzeStructure(const TokenList& tokenList, Metrics& metrics);

public:
    bool analyze(const TokenList& tokenList, Metrics& metrics);
};