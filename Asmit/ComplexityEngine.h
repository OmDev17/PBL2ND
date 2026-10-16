#ifndef COMPLEXITY_ENGINE_H
#define COMPLEXITY_ENGINE_H

// Forward declarations for Node and Result structures
struct Node;
struct Result;

class ComplexityEngine {
private:
    // Encapsulated internal logic (helper methods)
    int blockDegree(Node* first);
    int nodeDegree(Node* node);
    void collectStats(Node* first, int loopDepth, Result &r);
    void toBigO(int degree, char out[]);

public:
    // Constructor
    ComplexityEngine();

    // Main public method to evaluate code structure and calculate Big-O
    Result analyze(Node* root);

    // Public utility to compare two results (used for optimization / ranking)
    int compareResults(const Result &a, const Result &b);
};

#endif