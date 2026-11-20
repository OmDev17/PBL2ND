#include <iostream>
#include <fstream>
#include <string>

#include "../include/DataModels.h"
#include "../include/UniversalLexer.h"
#include "../include/LoopTreeEngine.h"
#include "../include/RecursionGraph.h"
#include "../include/StorageManager.h"

bool readFileToString(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) return false;
    out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close(); return true;
}

ComplexityResult mergeResults(const ComplexityResult& loopR, const ComplexityResult& recR) {
    ComplexityResult f = loopR;
    f.hasDirectRecursion = recR.hasDirectRecursion;
    f.hasIndirectRecursion = recR.hasIndirectRecursion;
    f.recursionType = recR.recursionType;
    if (recR.hasDirectRecursion || recR.hasIndirectRecursion) {
        if (recR.priorityRank > f.priorityRank) { f.bigO = recR.bigO; f.priorityRank = recR.priorityRank; }
        else if (f.loopCount == 0) { f.bigO = recR.bigO; f.priorityRank = recR.priorityRank; }
    }
    return f;
}

bool hasMeaningfulLogic(const TokenList& tokens) {
    int ops=0, ids=0, nums=0, delims=0;
    for (int i = 0; i < tokens.count; i++) {
        if (tokens.tokens[i].type == OPERATOR) ops++;
        if (tokens.tokens[i].type == IDENTIFIER) ids++;
        if (tokens.tokens[i].type == NUMBER) nums++;
        if (tokens.tokens[i].type == DELIMITER) delims++;
    }
    if (ops == 0 && delims == 0 && nums == 0 && ids == 0) return false;
    if (ops == 0 && delims <= 0 && nums == 0) return false;
    return (ops + ids + nums + delims) >= 2;
}

ComplexityResult analyzeSource(const std::string& source) {
    UniversalLexer lx;
    TokenList* tk = new TokenList();
    lx.tokenize(source.c_str(), *tk);
    LoopTreeEngine* le = new LoopTreeEngine();
    ComplexityResult lr; le->buildAndAnalyze(*tk, lr);
    RecursionGraph* rg = new RecursionGraph();
    ComplexityResult rr; rg->analyze(*tk, rr);
    ComplexityResult out = mergeResults(lr, rr);
    delete rg; delete le; delete tk;
    return out;
}

bool prepare(const std::string& source) {
    UniversalLexer lx; TokenList* tk = new TokenList();
    lx.tokenize(source.c_str(), *tk);
    bool ok = hasMeaningfulLogic(*tk);
    delete tk;
    if (!ok) {
        std::cout << "[Error] Incomplete or non-algorithmic input: "
                  << "No valid condition, bound, or statement body detected.\n";
    }
    return ok;
}

void printResult(const ComplexityResult& r) {
    std::cout << "\n--- Analysis Result (Time Only) ---\n";
    std::cout << "Big-O           : " << r.bigO << "\n";
    std::cout << "Priority Rank   : " << r.priorityRank << "\n";
    std::cout << "Loop Count      : " << r.loopCount << "\n";
    std::cout << "Max Loop Depth  : " << r.maxLoopDepth << "\n";
    std::cout << "Direct Recursion: " << (r.hasDirectRecursion ? "Yes" : "No") << "\n";
    std::cout << "Indirect Recurs.: " << (r.hasIndirectRecursion ? "Yes" : "No") << "\n";
    std::cout << "Recursion Type  : " << r.recursionType << "\n";
    std::cout << "-----------------------------------\n";
}

void pauseMenu() { std::cout << "\nPress Enter to return to menu..."; std::string d; std::getline(std::cin, d); }

int readIntChoice() {
    std::string line;
    if (!std::getline(std::cin, line)) { std::cin.clear(); return -1; }
    if (line.empty()) return -1;
    int v = 0;
    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] < '0' || line[i] > '9') return -1;
        v = v * 10 + (line[i] - '0');
    }
    return v;
}

std::string pickOrNewProject(StorageManager& store) {
    const int MAXP = 100;
    std::string* projects = new std::string[MAXP];
    int* counts = new int[MAXP];
    for (int i=0;i<MAXP;i++) counts[i]=0;
    int n = store.getUniqueProjects(projects, counts, MAXP);
    std::string out;
    if (n > 0) {
        std::cout << "Existing projects:\n";
        for (int i = 0; i < n; i++) std::cout << "  " << (i+1) << ". " << projects[i] << "\n";
        std::cout << "Pick number or 0 to type new name: ";
        int c = readIntChoice();
        if (c > 0 && c <= n) { out = projects[c-1]; delete[] projects; delete[] counts; return out; }
    }
    std::cout << "Enter project name: ";
    std::getline(std::cin, out);
    delete[] projects; delete[] counts; return out;
}

void optionAnalyzeSnippet(StorageManager& store) {
    std::string project = pickOrNewProject(store);
    if (project.empty()) { std::cout << "Project name required.\n"; return; }
    std::cout << "Enter code snippet (end with a line containing only END):\n";
    std::string source, line;
    while (std::getline(std::cin, line)) {
        if (line == "END") break;
        source += line; source += "\n";
    }
    if (source.empty()) { std::cout << "No code provided.\n"; return; }
    if (!prepare(source)) return;
    ComplexityResult r = analyzeSource(source);
    printResult(r);
    if (store.saveSubmission(project, r.bigO, r.priorityRank, r.maxLoopDepth, r.recursionType, source))
        std::cout << "Saved to submissions.txt\n";
}

void optionViewSummary(StorageManager& store) {
    const int MAXP = 100;
    std::string* projects = new std::string[MAXP];
    int* counts = new int[MAXP];
    for (int i=0;i<MAXP;i++) counts[i]=0;
    int n = store.getUniqueProjects(projects, counts, MAXP);
    if (n == 0) { std::cout << "No saved projects found in submissions.txt yet!\n"; delete[] projects; delete[] counts; return; }
    std::cout << "\nProject               | Subs | Last Big-O   | Optimal Big-O\n";
    std::cout << "------------------------------------------------------------\n";
    for (int i = 0; i < n; i++) {
        SubmissionRecord last, opt;
        store.findLastByProject(projects[i].c_str(), last);
        store.findMostOptimalByProject(projects[i].c_str(), opt);
        std::cout << projects[i];
        for (int s=(int)projects[i].size(); s<22; s++) std::cout << " ";
        std::cout << "| " << counts[i] << "    | " << last.bigO << "        | " << opt.bigO << "\n";
    }
    delete[] projects; delete[] counts;
}

void optionCompare(StorageManager& store) {
    std::cout << "Select First Project:\n";
    std::string p1 = store.selectProjectInteractive();
    if (p1.empty()) return;
    std::cout << "Select Second Project:\n";
    std::string p2 = store.selectProjectInteractive();
    if (p2.empty()) return;
    SubmissionRecord r1, r2;
    store.findMostOptimalByProject(p1.c_str(), r1);
    store.findMostOptimalByProject(p2.c_str(), r2);
    std::cout << "\n[1] " << p1 << " -> " << r1.bigO << " (rank " << r1.priorityRank << ")\n";
    std::cout << "[2] " << p2 << " -> " << r2.bigO << " (rank " << r2.priorityRank << ")\n";
    if (r1.priorityRank < r2.priorityRank) std::cout << "More optimal: " << p1 << "\n";
    else if (r2.priorityRank < r1.priorityRank) std::cout << "More optimal: " << p2 << "\n";
    else std::cout << "Both have equivalent complexity rank.\n";
}

void optionViewLast(StorageManager& store) {
    std::string p = store.selectProjectInteractive();
    if (p.empty()) return;
    SubmissionRecord rec;
    if (store.findLastByProject(p.c_str(), rec)) {
        std::cout << "\n--- Last Submitted Solution ---\n"; store.printRecord(rec, true);
    } else std::cout << "No submissions found for project.\n";
}

void optionViewOptimal(StorageManager& store) {
    std::string p = store.selectProjectInteractive();
    if (p.empty()) return;
    SubmissionRecord rec;
    if (store.findMostOptimalByProject(p.c_str(), rec)) {
        std::cout << "\n--- Most Optimal Solution ---\n"; store.printRecord(rec, true);
    } else std::cout << "No submissions found for project.\n";
}

void runDemoCase(StorageManager& store, int c) {
    const char* paths[] = {"test_cases/1_python_nested.py","test_cases/2_cpp_logarithmic.cpp",
                           "test_cases/3_fib_exponential.cpp","test_cases/4_indirect_recursion.txt"};
    const char* names[] = {"Demo_PythonNested","Demo_CppLog","Demo_Fibonacci","Demo_Indirect"};
    if (c < 1 || c > 4) return;
    std::string src;
    if (!readFileToString(paths[c-1], src)) { std::cout << "Cannot open " << paths[c-1] << "\n"; return; }
    if (!prepare(src)) return;
    ComplexityResult r = analyzeSource(src);
    std::cout << "\nDemo: " << paths[c-1] << "\n"; printResult(r);
    store.saveSubmission(names[c-1], r.bigO, r.priorityRank, r.maxLoopDepth, r.recursionType, src);
    std::cout << "Saved to submissions.txt\n";
}

void optionDemoSuite(StorageManager& store) {
    std::cout << "\n--- Built-in Demo Test Suite ---\n";
    std::cout << "  1. Python Nested Loop (O(n^2))\n";
    std::cout << "  2. C++ Logarithmic Loop (O(log n))\n";
    std::cout << "  3. Exponential Fibonacci (O(2^n))\n";
    std::cout << "  4. Mutual Indirect Recursion (O(n))\n";
    std::cout << "  5. Run All 4\n";
    std::cout << "Select (1-5, 0 cancel): ";
    int c = readIntChoice();
    if (c >= 1 && c <= 4) runDemoCase(store, c);
    else if (c == 5) { for (int i=1;i<=4;i++) runDemoCase(store, i); }
    else std::cout << "Cancelled.\n";
}

int main() {
    StorageManager store;
    while (true) {
        std::cout << "\n=========================================\n";
        std::cout << "      CODELENS-UNIVERSAL ANALYZER\n";
        std::cout << "=========================================\n";
        std::cout << "  1. Analyze & Save New Code Snippet\n";
        std::cout << "  2. View All Saved Projects & Submissions Summary\n";
        std::cout << "  3. Compare Two Saved Projects / Submissions (Pick by Number)\n";
        std::cout << "  4. View Last Submitted Solution (Pick Project by Number)\n";
        std::cout << "  5. View Most Optimal Solution (Pick Project by Number)\n";
        std::cout << "  6. Run Built-in Demo Test Suite (No File Path Needed)\n";
        std::cout << "  7. Exit\n";
        std::cout << "Choice: ";
        std::string choice;
        if (!std::getline(std::cin, choice)) { std::cin.clear(); continue; }
        if (choice == "1") optionAnalyzeSnippet(store);
        else if (choice == "2") optionViewSummary(store);
        else if (choice == "3") optionCompare(store);
        else if (choice == "4") optionViewLast(store);
        else if (choice == "5") optionViewOptimal(store);
        else if (choice == "6") optionDemoSuite(store);
        else if (choice == "7") { std::cout << "Goodbye.\n"; break; }
        else { std::cout << "Invalid choice.\n"; }
        if (choice != "7") pauseMenu();
    }
    return 0;
}
