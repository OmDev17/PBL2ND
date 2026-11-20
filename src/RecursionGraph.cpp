#include "../include/RecursionGraph.h"

RecursionGraph::RecursionGraph() { reset(); }

void RecursionGraph::reset() {
    funcCount = 0;
    for (int i = 0; i < MAX_FUNCTIONS; i++) {
        functions[i].name = "";
        functions[i].selfCallCount = 0;
        functions[i].halvesInput = false;
        for (int j = 0; j < MAX_FUNCTIONS; j++) adjMatrix[i][j] = 0;
    }
}

bool RecursionGraph::equalsIgnoreCase(const std::string& a, const char* b) const {
    int i = 0;
    while (i < (int)a.size() && b[i] != '\0') {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
        if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
        if (ca != cb) return false;
        i++;
    }
    return i == (int)a.size() && b[i] == '\0';
}

int RecursionGraph::findFunction(const std::string& name) const {
    for (int i = 0; i < funcCount; i++) if (functions[i].name == name) return i;
    return -1;
}

int RecursionGraph::addFunction(const std::string& name) {
    int idx = findFunction(name);
    if (idx != -1) return idx;
    if (funcCount >= MAX_FUNCTIONS) return -1;
    functions[funcCount].name = name;
    functions[funcCount].selfCallCount = 0;
    functions[funcCount].halvesInput = false;
    return funcCount++;
}

int RecursionGraph::findBlockEndBraces(const TokenList& tokens, int openBrace) const {
    int depth = 0;
    for (int i = openBrace; i < tokens.count; i++) {
        if (tokens.tokens[i].value == "{") depth++;
        else if (tokens.tokens[i].value == "}") { depth--; if (depth == 0) return i; }
    }
    return tokens.count - 1;
}

int RecursionGraph::findBlockEndIndent(const TokenList& tokens, int start, int baseIndent) const {
    for (int i = start; i < tokens.count; i++) {
        if (tokens.tokens[i].type == NEWLINE) continue;
        if (tokens.tokens[i].indentLevel <= baseIndent) return i - 1;
        if (equalsIgnoreCase(tokens.tokens[i].value, "end")) return i - 1;
    }
    return tokens.count - 1;
}

static int findMatchParen(const TokenList& tk, int open) {
    int depth = 0;
    for (int i = open; i < tk.count; i++) {
        if (tk.tokens[i].value == "(") depth++;
        else if (tk.tokens[i].value == ")") { depth--; if (depth == 0) return i; }
    }
    return -1;
}

// Pass 1: register all function names.
void RecursionGraph::detectDefinitions(const TokenList& tokens) {
    for (int i = 0; i < tokens.count - 1 && funcCount < MAX_FUNCTIONS; i++) {
        const Token& t = tokens.tokens[i];
        std::string fname = "";
        if (t.type == KEYWORD &&
            (equalsIgnoreCase(t.value, "def") || equalsIgnoreCase(t.value, "function") ||
             equalsIgnoreCase(t.value, "fn") || equalsIgnoreCase(t.value, "func"))) {
            if (i + 1 < tokens.count && tokens.tokens[i + 1].type == IDENTIFIER)
                fname = tokens.tokens[i + 1].value;
        }
        if (fname.empty() && t.type == IDENTIFIER && i + 1 < tokens.count &&
            tokens.tokens[i + 1].value == "(") {
            bool isControl = equalsIgnoreCase(t.value, "if") || equalsIgnoreCase(t.value, "for") ||
                             equalsIgnoreCase(t.value, "while") || equalsIgnoreCase(t.value, "switch") ||
                             equalsIgnoreCase(t.value, "catch") || equalsIgnoreCase(t.value, "foreach");
            if (!isControl) {
                int cp = findMatchParen(tokens, i + 1);
                if (cp != -1 && cp + 1 < tokens.count && tokens.tokens[cp + 1].value == "{")
                    fname = t.value;
            }
        }
        if (!fname.empty()) addFunction(fname);
    }
}

// Pass 2: locate bodies, fill adjMatrix, set halvesInput from call args.
void RecursionGraph::detectCalls(const TokenList& tokens) {
    int bStart[MAX_FUNCTIONS], bEnd[MAX_FUNCTIONS];
    for (int f = 0; f < funcCount; f++) { bStart[f] = -1; bEnd[f] = -1; }
    // locate body for each registered function
    for (int i = 0; i < tokens.count - 1; i++) {
        std::string fname = ""; int bs = -1, be = -1;
        const Token& t = tokens.tokens[i];
        if (t.type == KEYWORD &&
            (equalsIgnoreCase(t.value, "def") || equalsIgnoreCase(t.value, "function") ||
             equalsIgnoreCase(t.value, "fn") || equalsIgnoreCase(t.value, "func"))) {
            if (i + 1 < tokens.count && tokens.tokens[i + 1].type == IDENTIFIER) {
                fname = tokens.tokens[i + 1].value;
                for (int j = i + 2; j < tokens.count; j++) {
                    if (tokens.tokens[j].value == "{") { bs = j + 1; be = findBlockEndBraces(tokens, j); break; }
                    if (tokens.tokens[j].value == ":") { bs = j + 1; be = findBlockEndIndent(tokens, j + 1, t.indentLevel); break; }
                    if (tokens.tokens[j].type == NEWLINE) {
                        bs = j + 1; be = tokens.count - 1;
                        for (int k = j + 1; k < tokens.count; k++) {
                            if (equalsIgnoreCase(tokens.tokens[k].value, "end") && tokens.tokens[k].indentLevel <= t.indentLevel) { be = k - 1; break; }
                        }
                        break;
                    }
                }
            }
        }
        if (fname.empty() && t.type == IDENTIFIER && i + 1 < tokens.count && tokens.tokens[i + 1].value == "(") {
            bool isControl = equalsIgnoreCase(t.value, "if") || equalsIgnoreCase(t.value, "for") ||
                             equalsIgnoreCase(t.value, "while") || equalsIgnoreCase(t.value, "switch") ||
                             equalsIgnoreCase(t.value, "catch") || equalsIgnoreCase(t.value, "foreach");
            if (!isControl) {
                int cp = findMatchParen(tokens, i + 1);
                if (cp != -1 && cp + 1 < tokens.count && tokens.tokens[cp + 1].value == "{") {
                    fname = t.value; bs = cp + 2; be = findBlockEndBraces(tokens, cp + 1);
                }
            }
        }
        if (!fname.empty()) {
            int idx = findFunction(fname);
            if (idx != -1 && bStart[idx] == -1) { bStart[idx] = bs; bEnd[idx] = be; }
            if (be >= 0) i = be;
        }
    }
    // scan calls inside bodies
    for (int f = 0; f < funcCount; f++) {
        if (bStart[f] == -1) continue;
        for (int k = bStart[f]; k <= bEnd[f] && k < tokens.count - 1; k++) {
            if (tokens.tokens[k].type != IDENTIFIER || tokens.tokens[k + 1].value != "(") continue;
            int callee = findFunction(tokens.tokens[k].value);
            if (callee == -1) continue;
            int cp = findMatchParen(tokens, k + 1);
            if (cp == -1) continue;
            adjMatrix[f][callee]++;
            if (f == callee) functions[f].selfCallCount++;
            // check halving ONLY inside recursive call args
            bool halve = false;
            for (int a = k + 2; a < cp; a++) {
                const std::string& v = tokens.tokens[a].value;
                if (v=="/"||v=="/="||v==">>"||v==">>=") { halve = true; break; }
                if (equalsIgnoreCase(v, "mid")) { halve = true; break; }
            }
            if (f == callee && halve) functions[f].halvesInput = true;
            k = cp;
        }
    }
}

bool RecursionGraph::dfsCycle(int u, bool visited[], bool recStack[], bool& indirect) const {
    visited[u] = true; recStack[u] = true;
    for (int v = 0; v < funcCount; v++) {
        if (adjMatrix[u][v] <= 0 || u == v) continue;
        if (!visited[v]) { if (dfsCycle(v, visited, recStack, indirect)) return true; }
        else if (recStack[v]) { indirect = true; return true; }
    }
    recStack[u] = false; return false;
}

void RecursionGraph::analyzeRecursion(ComplexityResult& result) const {
    bool direct = false, indirect = false;
    for (int i = 0; i < funcCount; i++) if (adjMatrix[i][i] > 0) direct = true;
    bool visited[MAX_FUNCTIONS], recStackArr[MAX_FUNCTIONS];
    for (int i = 0; i < funcCount; i++) { visited[i] = false; recStackArr[i] = false; }
    for (int i = 0; i < funcCount; i++) if (!visited[i]) dfsCycle(i, visited, recStackArr, indirect);
    result.hasDirectRecursion = direct;
    result.hasIndirectRecursion = indirect;
    if (!direct && !indirect) { result.recursionType = "None"; return; }
    int maxCalls = 0; bool anyHalving = false;
    for (int i = 0; i < funcCount; i++) {
        for (int j = 0; j < funcCount; j++) if (adjMatrix[i][j] > maxCalls) maxCalls = adjMatrix[i][j];
        if (functions[i].halvesInput) anyHalving = true;
    }
    if (maxCalls >= 2) {
        result.recursionType = "Multiple-branch (Exponential)";
        result.bigO = "O(2^n)"; result.priorityRank = 8;
    } else if (anyHalving) {
        result.recursionType = (direct && indirect) ? "Direct+Indirect (Logarithmic)" :
                              direct ? "Direct (Logarithmic)" : "Indirect (Logarithmic)";
        result.bigO = "O(log n)"; result.priorityRank = 2;
    } else {
        result.recursionType = (direct && indirect) ? "Direct+Indirect (Linear)" :
                              direct ? "Direct (Linear)" : "Indirect (Linear)";
        result.bigO = "O(n)"; result.priorityRank = 3;
    }
}

void RecursionGraph::analyze(const TokenList& tokens, ComplexityResult& result) {
    reset();
    result.hasDirectRecursion = false;
    result.hasIndirectRecursion = false;
    result.recursionType = "None";
    detectDefinitions(tokens); // pass 1: register names
    detectCalls(tokens);       // pass 2: bodies, calls, adjMatrix
    analyzeRecursion(result);
}
