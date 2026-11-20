#ifndef LOOP_TREE_ENGINE_H
#define LOOP_TREE_ENGINE_H

#include "DataModels.h"

// Builds an N-ary loop tree and computes iterative time complexity.
class LoopTreeEngine {
private:
    LoopNode nodePool[MAX_TOKENS];
    int poolUsed;

    LoopNode* newNode(LoopBound bound);
    bool eqi(const std::string& a, const char* b) const;
    bool isLoop(const Token& t) const;
    bool isCmpOp(const std::string& v) const;
    bool isExecToken(const Token& t) const;
    int findMatch(const TokenList& tk, int open, const char* o, const char* c) const;
    bool hasValidHeader(const TokenList& tk, int hs, int he) const;
    bool hasValidBody(const TokenList& tk, int bs, int be) const;
    bool isLogPattern(const TokenList& tk, int s, int e) const;
    bool hasMidUpdate(const TokenList& tk, int s, int e) const;
    bool condVarUpdated(const TokenList& tk, int hs, int he, int bs, int be) const;
    bool headerHasVarBound(const TokenList& tk, int hs, int he) const;
    LoopBound classifyBound(const TokenList& tk, int hs, int he, int bs, int be) const;
    int buildBraces(const TokenList& tk, int s, int e, LoopNode* p);
    int buildIndent(const TokenList& tk, int s, int baseIndent, LoopNode* p);
    int rankOf(int lin, int log) const;
    std::string bigOOf(int lin, int log) const;
    void freeNode(LoopNode* node);

public:
    LoopTreeEngine();
    void reset();
    void buildAndAnalyze(const TokenList& tokens, ComplexityResult& result);
};

#endif // LOOP_TREE_ENGINE_H
