#ifndef COMPLEXITY_MODEL_H
#define COMPLEXITY_MODEL_H

enum NodeType {
    SIMPLE,
    LOOP,
    IF_STMT
};

enum BoundType {
    CONSTANT_BOUND,
    N_BOUND
};

struct Node {
    NodeType type;
    BoundType bound;
    Node* firstChild;
    Node* elseChild;
    Node* nextSibling;
};

struct Result {
    int loops;
    int ifs;
    int maxLoopDepth;
    int degree;
    char bigO[30];
};

#endif