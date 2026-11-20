// Edge-case tests for RecursionGraph.
// Build: g++ -std=c++11 -Wall -Iinclude src/RecursionGraph.cpp test_cases/test_recursion_graph.cpp -o test_rg
#include "../include/RecursionGraph.h"
#include <iostream>
#include <cctype>

// ---- Minimal lexer so tests can be written as source text.
// If your project already has a lexer, replace lex() with a call to it.
static bool isKw(const std::string& s) {
    return s == "def" || s == "function" || s == "fn" || s == "func";
}
static TokenList lex(const std::string& src) {
    TokenList tl;
    int indent = 0, line = 1; bool lineStart = true; size_t i = 0;
    auto add = [&](TokenType t, const std::string& v) {
        tl.tokens[tl.count].type = t; tl.tokens[tl.count].value = v;
        tl.tokens[tl.count].indentLevel = indent; tl.tokens[tl.count].line = line; tl.count++;
    };
    while (i < src.size()) {
        char c = src[i];
        if (c == '\n') { add(NEWLINE, "\n"); line++; indent = 0; lineStart = true; i++; }
        else if (c == ' ' && lineStart) { int sp = 0; while (i < src.size() && src[i] == ' ') { sp++; i++; } indent = sp / 4; lineStart = false; }
        else if (c == ' ' || c == '\t' || c == '\r') { i++; lineStart = false; }
        else if (isalpha((unsigned char)c) || c == '_') {
            size_t j = i; while (j < src.size() && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;
            std::string w = src.substr(i, j - i); add(isKw(w) ? KEYWORD : IDENTIFIER, w); i = j; lineStart = false;
        } else if (isdigit((unsigned char)c)) {
            size_t j = i; while (j < src.size() && isdigit((unsigned char)src[j])) j++;
            add(NUMBER, src.substr(i, j - i)); i = j; lineStart = false;
        } else {
            std::string two = src.substr(i, 2);
            if (two == ">>" || two == "/=" || two == "->") { add(OPERATOR, two); i += 2; }
            else {
                bool delim = (c == '(' || c == ')' || c == '{' || c == '}' || c == ':' || c == ';' || c == ',');
                add(delim ? DELIMITER : OPERATOR, std::string(1, c)); i++;
            }
            lineStart = false;
        }
    }
    return tl;
}

static int passed = 0, failed = 0;
#define CHECK(name, cond) do { if (cond) { passed++; std::cout << "[PASS] " << name << "\n"; } \
    else { failed++; std::cout << "[FAIL] " << name << "\n"; } } while (0)

static ComplexityResult run(const std::string& src) {
    static RecursionGraph g; ComplexityResult r; TokenList tl = lex(src); g.analyze(tl, r); return r;
}

int main() {
    ComplexityResult r;

    // ---------- Basic / empty input ----------
    { RecursionGraph g; ComplexityResult x; TokenList tl; g.analyze(tl, x);
      CHECK("empty token list -> None, no crash", x.recursionType == "None" && !x.hasDirectRecursion && !x.hasIndirectRecursion); }
    r = run("int x = 5;");
    CHECK("no functions -> None", r.recursionType == "None");
    r = run("int add(int a, int b) { return a + b; }");
    CHECK("non-recursive function -> None", r.recursionType == "None" && !r.hasDirectRecursion);

    // ---------- Direct recursion ----------
    r = run("int fact(int n) { if (n <= 1) { return 1; } return n * fact(n - 1); }");
    CHECK("factorial -> Direct (Linear)", r.hasDirectRecursion && r.recursionType == "Direct (Linear)" && r.bigO == "O(n)");
    r = run("int fib(int n) { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); }");
    CHECK("fibonacci -> Exponential", r.recursionType == "Multiple-branch (Exponential)" && r.bigO == "O(2^n)");
    r = run("int pw(int b, int n) { if (n == 0) { return 1; } return pw(b, n / 2) * b; }");
    CHECK("n/2 argument -> Logarithmic", r.recursionType == "Direct (Logarithmic)" && r.bigO == "O(log n)");
    r = run("int f(int n) { return f(n >> 1); }");
    CHECK(">> argument -> Logarithmic", r.bigO == "O(log n)");

    // ---------- Indirect recursion ----------
    r = run("int a(int n) { return b(n - 1); } int b(int n) { return a(n - 1); }");
    CHECK("mutual recursion a<->b -> indirect only", r.hasIndirectRecursion && !r.hasDirectRecursion && r.recursionType == "Indirect (Linear)");
    r = run("int a(int n) { return b(n); } int b(int n) { return c(n); } int c(int n) { return a(n); }");
    CHECK("3-cycle a->b->c->a -> indirect", r.hasIndirectRecursion);
    r = run("int a(int n) { return a(n-1) + b(n); } int b(int n) { return a(n-1); }");
    CHECK("self + mutual -> Direct+Indirect", r.hasDirectRecursion && r.hasIndirectRecursion);
    r = run("int a(int n) { return b(n); } int b(int n) { return n; }");
    CHECK("a calls b, no cycle -> None", r.recursionType == "None");

    // ---------- Graph-structure edge cases ----------
    r = run("int a(int n) { return b(n); } int b(int n) { return c(n); } int c(int n) { return n; } int d(int n) { return b(n); }");
    CHECK("diamond / DAG without cycle -> None", !r.hasIndirectRecursion && !r.hasDirectRecursion);
    r = run("int a(int n) { return n; } int r(int n) { return r(n - 1); }");
    CHECK("recursive function after non-recursive one is found", r.hasDirectRecursion);

    // ---------- Syntax styles ----------
    r = run("def f(n):\n    return f(n - 1)\n");
    CHECK("python-style def with colon", r.hasDirectRecursion);
    r = run("function f(n) { return f(n - 1); }");
    CHECK("'function' keyword with braces", r.hasDirectRecursion);
    r = run("fn f(n) { return f(n - 1); }");
    CHECK("'fn' keyword", r.hasDirectRecursion);

    // ---------- Robustness: malformed input must not crash ----------
    r = run("int f(int n) { return f(n - 1);");
    CHECK("missing closing brace -> no crash, still detects", r.hasDirectRecursion);
    r = run("int f(int n) { return f(n - 1 ");
    CHECK("truncated call parens (reaching here = no crash)", true);
    r = run("int f(");
    CHECK("function header cut off -> None", r.recursionType == "None");
    r = run("def");
    CHECK("lone 'def' keyword -> None", r.recursionType == "None");
    r = run("if (x) { while (y) { } }");
    CHECK("control keywords are not treated as functions", r.recursionType == "None");

    // ---------- Capacity ----------
    { std::string big;
      for (int i = 0; i < MAX_FUNCTIONS + 5; i++) big += "int f" + std::to_string(i) + "(int n) { return n; } ";
      big += "int self(int n) { return self(n-1); }";
      r = run(big);
      CHECK("more than MAX_FUNCTIONS functions (reaching here = no crash)", true); }

    // ---------- State reset between runs ----------
    r = run("int f(int n) { return f(n - 1); }");
    r = run("int g(int n) { return n; }");
    CHECK("analyze() resets state between runs", r.recursionType == "None" && !r.hasDirectRecursion);

    // ---------- Defaults for non-recursive code ----------
    r = run("int add(int a, int b) { return a + b; }");
    CHECK("non-recursive keeps default O(1) / rank 1", r.bigO == "O(1)" && r.priorityRank == 1);

    // ---------- KNOWN BUGS (these fail until the code is fixed) ----------
    r = run("int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); } int main() { fact(5); fact(10); return 0; }");
    CHECK("BUG: main calling linear fact() twice must stay O(n)", r.bigO == "O(n)");

    r = run("int add(int a, int b) { return a + b; } int fib(int n) { return add(fib(n - 1), fib(n - 2)); }");
    CHECK("BUG: recursive calls nested inside another user function's args are detected", r.hasDirectRecursion);

    r = run("def f(n: int):\n    return f(n - 1)\n");
    CHECK("BUG: python type-annotated params (colon inside parens)", r.hasDirectRecursion);

    r = run("def f(n): return f(n - 1)\n");
    CHECK("BUG: one-line python def", r.hasDirectRecursion);

    r = run("int bs(int lo, int hi) { if (lo > hi) { return 0; } int mid = lo; if (mid) { return bs(lo, mid - 1); } return bs(mid + 1, hi); }");
    CHECK("BUG: binary search (two exclusive branches, halving) should be O(log n)", r.bigO == "O(log n)");

    r = run("int f(int n) const { return f(n - 1); }");
    CHECK("BUG: C++ 'const' member qualifier before '{'", r.hasDirectRecursion);

    r = run("int a(int n) { return b(n / 2); } int b(int n) { return a(n / 2); }");
    CHECK("BUG: halving through mutual recursion should be Logarithmic", r.bigO == "O(log n)");

    { RecursionGraph g; ComplexityResult shared; TokenList a = lex("int fib(int n) { return fib(n-1) + fib(n-2); }");
      g.analyze(a, shared);
      TokenList b = lex("int add(int x, int y) { return x + y; }");
      g.analyze(b, shared);
      CHECK("BUG: reusing a ComplexityResult must not keep stale bigO from a previous run",
            shared.recursionType == "None" && shared.bigO == "O(1)" && shared.priorityRank == 1); }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}