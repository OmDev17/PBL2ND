#include "../include/LoopTreeEngine.h"

LoopTreeEngine::LoopTreeEngine() { poolUsed = 0; }
void LoopTreeEngine::reset() { poolUsed = 0; }

LoopNode* LoopTreeEngine::newNode(LoopBound bound) {
    if (poolUsed >= MAX_TOKENS) return 0;
    LoopNode* n = &nodePool[poolUsed++];
    n->bound = bound;
    n->childCount = 0;
    for (int i = 0; i < 20; i++) n->children[i] = 0;
    return n;
}

bool LoopTreeEngine::eqi(const std::string& a, const char* b) const {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
        if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
        if (ca != cb) return false;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

bool LoopTreeEngine::isLoop(const Token& t) const {
    if (t.type != KEYWORD) return false;
    return eqi(t.value, "for") || eqi(t.value, "while") ||
           eqi(t.value, "loop") || eqi(t.value, "repeat") || eqi(t.value, "foreach");
}

bool LoopTreeEngine::isCmpOp(const std::string& v) const {
    return v == "<" || v == "<=" || v == ">" || v == ">=" || v == "!=" ||
           v == "==" || eqi(v, "in") || eqi(v, "to") || v == ":" || v == ";";
}

bool LoopTreeEngine::isExecToken(const Token& t) const {
    return t.type == IDENTIFIER || t.type == KEYWORD || t.type == OPERATOR;
}

int LoopTreeEngine::findMatch(const TokenList& tk, int open,
                              const char* o, const char* c) const {
    int depth = 0;
    for (int i = open; i < tk.count; i++) {
        if (tk.tokens[i].value == o) depth++;
        else if (tk.tokens[i].value == c) { depth--; if (depth == 0) return i; }
    }
    return -1;
}

// Header must have >=2 logical tokens forming a range/condition.
bool LoopTreeEngine::hasValidHeader(const TokenList& tk, int hs, int he) const {
    bool hasId = false, hasCond = false;
    if (hs < 0) hs = 0;
    if (he > tk.count) he = tk.count;
    for (int i = hs + 1; i < he; i++) {
        const Token& t = tk.tokens[i];
        if (t.type == IDENTIFIER || t.type == NUMBER) hasId = true;
        if (isCmpOp(t.value) || t.type == OPERATOR) hasCond = true;
    }
    return hasId && hasCond;
}

// Body must contain >=1 executable token.
bool LoopTreeEngine::hasValidBody(const TokenList& tk, int bs, int be) const {
    if (bs < 0) bs = 0;
    if (be > tk.count) be = tk.count;
    for (int i = bs; i < be; i++) {
        if (isExecToken(tk.tokens[i])) return true;
    }
    return false;
}

bool LoopTreeEngine::isLogPattern(const TokenList& tk, int s, int e) const {
    if (s < 0) s = 0;
    if (e > tk.count) e = tk.count;
    for (int i = s; i < e; i++) {
        const std::string& v = tk.tokens[i].value;
        if (v == "*=" || v == "/=" || v == ">>=" || v == "<<=") return true;
        if (v == ">>" || v == "<<") return true;
        if (v == "=" && i + 3 < e && (tk.tokens[i+2].value=="*" || tk.tokens[i+2].value=="/")) return true;
    }
    return false;
}

bool LoopTreeEngine::hasMidUpdate(const TokenList& tk, int s, int e) const {
    if (s < 0) s = 0;
    if (e > tk.count) e = tk.count;
    for (int i = s; i < e - 2; i++) {
        if (eqi(tk.tokens[i].value, "mid") &&
            (tk.tokens[i+1].value == "+" || tk.tokens[i+1].value == "-") &&
            tk.tokens[i+2].value == "1") return true;
    }
    return false;
}

// While-loop condition variable must be updated in body.
bool LoopTreeEngine::condVarUpdated(const TokenList& tk, int hs, int he, int bs, int be) const {
    if (hs < 0) hs = 0;
    if (he > tk.count) he = tk.count;
    if (bs < 0) bs = 0;
    if (be > tk.count) be = tk.count;
    for (int i = hs + 1; i < he; i++) {
        if (tk.tokens[i].type != IDENTIFIER) continue;
        const std::string& var = tk.tokens[i].value;
        if (eqi(var, "mid")) continue;
        for (int j = bs; j < be; j++) {
            if (tk.tokens[j].type != IDENTIFIER || tk.tokens[j].value != var) continue;
            if (j+1 < be) {
                const std::string& n = tk.tokens[j+1].value;
                if (n=="++"||n=="--"||n=="+="||n=="-="||n=="*="||n=="/="||n=="=") return true;
            }
            if (j>bs && (tk.tokens[j-1].value=="++" || tk.tokens[j-1].value=="--")) return true;
        }
    }
    return hasMidUpdate(tk, bs, be);
}

// Header has a variable IDENTIFIER acting as bound (not just number literals).
bool LoopTreeEngine::headerHasVarBound(const TokenList& tk, int hs, int he) const {
    if (hs < 0) hs = 0;
    if (he > tk.count) he = tk.count;
    bool sawCmp = false;
    for (int i = hs + 1; i < he; i++) {
        const Token& t = tk.tokens[i];
        bool cmp = (t.value=="<"||t.value=="<="||t.value==">"||t.value==">="||t.value=="!=");
        if (cmp) {
            sawCmp = true;
            // bound is typically on the right side of comparison
            if (i + 1 < he && tk.tokens[i + 1].type == IDENTIFIER) return true;
        }
        if (eqi(t.value,"in") || eqi(t.value,"to")) {
            sawCmp = true;
            for (int j = i + 1; j < he; j++) {
                if (tk.tokens[j].type == IDENTIFIER && !eqi(tk.tokens[j].value, "range")) return true;
                if (tk.tokens[j].type == NUMBER) return false;
            }
        }
        if (eqi(t.value,"range") && i + 2 < he && tk.tokens[i + 1].value == "(") {
            sawCmp = true;
            // range(stop) or range(start, stop, step): variable if any arg is IDENTIFIER
            for (int j = i + 2; j < he && tk.tokens[j].value != ")"; j++) {
                if (tk.tokens[j].type == IDENTIFIER) return true;
            }
            return false;
        }
    }
    if (!sawCmp) return true;
    return false;
}

LoopBound LoopTreeEngine::classifyBound(const TokenList& tk, int hs, int he,
                                        int bs, int be) const {
    if (isLogPattern(tk, hs, he) || isLogPattern(tk, bs, be) || hasMidUpdate(tk, bs, be))
        return LOGARITHMIC;
    if (!headerHasVarBound(tk, hs, he)) return CONSTANT;
    return LINEAR;
}

// Build loop tree for brace-based code.
int LoopTreeEngine::buildBraces(const TokenList& tk, int s, int e, LoopNode* p) {
    int i = s;
    while (i < e && i < tk.count) {
        const Token& t = tk.tokens[i];
        if (isLoop(t)) {
            int hEnd = i + 1;
            if (i+1 < tk.count && tk.tokens[i+1].value == "(") {
                int cl = findMatch(tk, i+1, "(", ")");
                if (cl != -1) hEnd = cl + 1;
            }
            // body detection
            int bStart = hEnd, bEnd = hEnd;
            bool braceBody = (bStart < e && tk.tokens[bStart].value == "{");
            if (braceBody) {
                int cb = findMatch(tk, bStart, "{", "}");
                bEnd = (cb != -1 && cb < e) ? cb : bStart;
            } else {
                int se = bStart;
                while (se < e && se < tk.count) {
                    if (tk.tokens[se].value==";" || tk.tokens[se].type==NEWLINE) { se++; break; }
                    if (tk.tokens[se].value=="{" || tk.tokens[se].value=="}") break;
                    se++;
                }
                bEnd = (se == bStart ? bStart + 1 : se);
            }
            bool validHdr = hasValidHeader(tk, i, hEnd);
            int bodyTokS = braceBody ? bStart + 1 : bStart;
            int bodyTokE = braceBody ? bEnd : bEnd;
            bool validBody = hasValidBody(tk, bodyTokS, bodyTokE);
            bool whileOk = true;
            if (eqi(t.value, "while") && (!validBody || !condVarUpdated(tk, i, hEnd, bodyTokS, bodyTokE)))
                whileOk = false;
            if (validHdr && validBody && whileOk) {
                LoopBound bound = classifyBound(tk, i, hEnd, bodyTokS, bodyTokE);
                LoopNode* ln = newNode(bound);
                if (ln && p->childCount < 20) {
                    p->children[p->childCount++] = ln;
                    if (braceBody && bEnd > bStart) buildBraces(tk, bStart + 1, bEnd, ln);
                    else buildBraces(tk, bStart, bEnd, ln);
                }
            } else {
                // invalid loop: still recurse into body to find nested valid loops
                if (braceBody && bEnd > bStart) buildBraces(tk, bStart + 1, bEnd, p);
                else buildBraces(tk, bStart, bEnd, p);
            }
            i = braceBody ? (bEnd + 1) : bEnd;
            continue;
        } else if (t.value == "{") {
            int cb = findMatch(tk, i, "{", "}");
            if (cb != -1 && cb < e) { buildBraces(tk, i + 1, cb, p); i = cb + 1; continue; }
        }
        i++;
    }
    return i;
}

// Build loop tree for indentation-based (Python/pseudocode) code.
int LoopTreeEngine::buildIndent(const TokenList& tk, int s, int baseIndent, LoopNode* p) {
    int i = s;
    while (i < tk.count) {
        const Token& t = tk.tokens[i];
        if (t.type == NEWLINE) { i++; continue; }
        if (t.indentLevel < baseIndent) return i;
        if (isLoop(t)) {
            int headerLine = t.line, loopIndent = t.indentLevel;
            int hEnd = i + 1;
            while (hEnd < tk.count && tk.tokens[hEnd].line == headerLine) hEnd++;
            int bodyIndent = loopIndent + 1;
            for (int k = hEnd; k < tk.count; k++) {
                if (tk.tokens[k].type == NEWLINE) continue;
                if (tk.tokens[k].line > headerLine) { bodyIndent = tk.tokens[k].indentLevel; break; }
            }
            int bStart = hEnd, bEnd = bStart, k = bStart;
            while (k < tk.count) {
                if (tk.tokens[k].type == NEWLINE) { k++; continue; }
                if (tk.tokens[k].line <= headerLine) { k++; continue; }
                if (tk.tokens[k].indentLevel < bodyIndent) break;
                bEnd = k + 1; k++;
            }
            bool validHdr = hasValidHeader(tk, i, hEnd);
            bool validBody = hasValidBody(tk, bStart, bEnd);
            bool whileOk = true;
            if (eqi(t.value, "while") && (!validBody || !condVarUpdated(tk, i, hEnd, bStart, bEnd)))
                whileOk = false;
            if (validHdr && validBody && whileOk) {
                LoopBound bound = classifyBound(tk, i, hEnd, bStart, bEnd);
                LoopNode* ln = newNode(bound);
                if (ln && p->childCount < 20) {
                    p->children[p->childCount++] = ln;
                    if (bEnd > bStart) buildIndent(tk, bStart, bodyIndent, ln);
                }
            } else {
                if (bEnd > bStart) buildIndent(tk, bStart, bodyIndent, p);
            }
            i = (bEnd > bStart) ? bEnd : hEnd;
            continue;
        }
        // non-loop line at/above: skip line, descend into deeper block with same parent
        int line = t.line;
        while (i < tk.count && tk.tokens[i].line == line) i++;
        int j = i; while (j < tk.count && tk.tokens[j].type == NEWLINE) j++;
        if (j < tk.count && tk.tokens[j].indentLevel > baseIndent) {
            i = buildIndent(tk, j, tk.tokens[j].indentLevel, p); continue;
        }
        i = j;
    }
    return i;
}

int LoopTreeEngine::rankOf(int lin, int log) const {
    if (lin <= 0 && log <= 0) return 1;
    if (lin == 0 && log >= 1) return 2;
    if (lin == 1 && log == 0) return 3;
    if (lin == 1 && log >= 1) return 4;
    if (lin == 2 && log == 0) return 5;
    if (lin == 2 && log >= 1) return 6;
    if (lin == 3 && log == 0) return 7;
    return 7 + (lin - 3);
}

std::string LoopTreeEngine::bigOOf(int lin, int log) const {
    if (lin <= 0 && log <= 0) return "O(1)";
    if (lin == 0 && log >= 1) return "O(log n)";
    if (lin == 1 && log == 0) return "O(n)";
    if (lin == 1 && log >= 1) return "O(n log n)";
    if (lin == 2 && log == 0) return "O(n^2)";
    if (lin == 2 && log >= 1) return "O(n^2 log n)";
    if (lin == 3 && log == 0) return "O(n^3)";
    char buf[32]; int p = 0;
    const char* pre = "O(n^"; while (*pre) buf[p++] = *pre++;
    char tmp[8]; int t2 = 0, k = lin; if (k == 0) tmp[t2++] = '0';
    while (k > 0) { tmp[t2++] = char('0' + k % 10); k /= 10; }
    for (int x = t2 - 1; x >= 0; x--) buf[p++] = tmp[x];
    if (log >= 1) { const char* suf=" log n)"; while(*suf) buf[p++]=*suf++; }
    else buf[p++] = ')';
    buf[p] = '\0';
    return std::string(buf);
}

void LoopTreeEngine::freeNode(LoopNode* node) {
    if (!node) return;
    for (int i = 0; i < node->childCount; i++) freeNode(node->children[i]);
    node->childCount = 0;
}

void LoopTreeEngine::buildAndAnalyze(const TokenList& tokens, ComplexityResult& result) {
    reset();
    bool braceStyle = false;
    for (int i = 0; i < tokens.count; i++) { if (tokens.tokens[i].value == "{") { braceStyle = true; break; } }
    LoopNode* root = newNode(CONSTANT);
    if (!root) return;
    if (braceStyle) buildBraces(tokens, 0, tokens.count, root);
    else buildIndent(tokens, 0, 0, root);

    struct Frame { LoopNode* node; int idx; int lin; int log; int depth; };
    Frame* stack = new Frame[MAX_TOKENS];
    int top = 0, bestRank = 1, bestLin = 0, bestLog = 0, maxDepth = 0, totalLoops = 0;
    stack[top++] = {root, 0, 0, 0, 0};
    while (top > 0) {
        Frame& f = stack[top - 1];
        if (f.idx >= f.node->childCount) {
            if (f.node != root) {
                int r = rankOf(f.lin, f.log);
                if (r > bestRank) { bestRank = r; bestLin = f.lin; bestLog = f.log; }
            }
            top--; continue;
        }
        LoopNode* child = f.node->children[f.idx++];
        if (!child) continue;
        totalLoops++;
        int nlin = f.lin + (child->bound == LINEAR ? 1 : 0);
        int nlog = f.log + (child->bound == LOGARITHMIC ? 1 : 0);
        int ndepth = f.depth + 1;
        if (ndepth > maxDepth) maxDepth = ndepth;
        if (top < MAX_TOKENS) stack[top++] = {child, 0, nlin, nlog, ndepth};
    }
    delete[] stack;
    if (totalLoops == 0) { bestRank = 1; bestLin = 0; bestLog = 0; }
    result.loopCount = totalLoops;
    result.maxLoopDepth = maxDepth;
    result.bigO = bigOOf(bestLin, bestLog);
    result.priorityRank = rankOf(bestLin, bestLog);
    reset();
}
