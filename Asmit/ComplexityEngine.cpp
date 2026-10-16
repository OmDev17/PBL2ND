#include "ComplexityModel.h"
#include "ComplexityEngine.h"
#include <cstring>
#include <cstdio>

// Constructor implementation
ComplexityEngine::ComplexityEngine() {
    // Initialization logic if needed
}

// Sequential statements: the dominant (largest) term wins
int ComplexityEngine::blockDegree(Node* first) {
    int worst = 0;
    for (Node* cur = first; cur != NULL; cur = cur->nextSibling) {
        int d = nodeDegree(cur);
        if (d > worst) worst = d;
    }
    return worst;
}

int ComplexityEngine::nodeDegree(Node* node) {
    if (node->type == LOOP) {
        int own = (node->bound == N_BOUND) ? 1 : 0;
        return own + blockDegree(node->firstChild);   // nested = multiply = add degrees
    }
    if (node->type == IF_STMT) {
        int thenD = blockDegree(node->firstChild);
        int elseD = blockDegree(node->elseChild);
        return thenD > elseD ? thenD : elseD;         // worst-case branch
    }
    return 0;                                       // simple statement = O(1)
}

void ComplexityEngine::collectStats(Node* first, int loopDepth, Result &r) {
    for (Node* cur = first; cur != NULL; cur = cur->nextSibling) {
        if (cur->type == LOOP) {
            r.loops++;
            int d = loopDepth + 1;
            if (d > r.maxLoopDepth) r.maxLoopDepth = d;
            collectStats(cur->firstChild, d, r);
        } else if (cur->type == IF_STMT) {
            r.ifs++;
            collectStats(cur->firstChild, loopDepth, r);
            collectStats(cur->elseChild, loopDepth, r);
        }
    }
}

void ComplexityEngine::toBigO(int degree, char out[]) {
    if (degree == 0)      strcpy(out, "O(1)");
    else if (degree == 1) strcpy(out, "O(n)");
    else                  sprintf(out, "O(n^%d)", degree);
}

Result ComplexityEngine::analyze(Node* root) {
    Result r;
    r.loops = 0; r.ifs = 0; r.maxLoopDepth = 0;
    r.degree = blockDegree(root->firstChild);
    collectStats(root->firstChild, 0, r);
    toBigO(r.degree, r.bigO);
    return r;
}

// -1 = A better, 1 = B better, 0 = tie
int ComplexityEngine::compareResults(const Result &a, const Result &b) {
    if (a.degree != b.degree)
        return a.degree < b.degree ? -1 : 1;
    if (a.maxLoopDepth != b.maxLoopDepth)
        return a.maxLoopDepth < b.maxLoopDepth ? -1 : 1;
    return 0;
}