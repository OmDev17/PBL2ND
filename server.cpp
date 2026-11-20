#include "include/DataModels.h"
#include "include/UniversalLexer.h"
#include "include/LoopTreeEngine.h"
#include "include/RecursionGraph.h"
#include "include/StorageManager.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// --- Analyzer Helpers (Exact logic from main.cpp) ---

bool readFileToString(const std::string& path, std::string& out) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in.is_open()) return false;
    out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    return true;
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
    int ops = 0, ids = 0, nums = 0, delims = 0;
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
    ComplexityResult lr;
    le->buildAndAnalyze(*tk, lr);
    RecursionGraph* rg = new RecursionGraph();
    ComplexityResult rr;
    rg->analyze(*tk, rr);
    ComplexityResult out = mergeResults(lr, rr);
    delete rg;
    delete le;
    delete tk;
    return out;
}

bool prepare(const std::string& source) {
    UniversalLexer lx;
    TokenList* tk = new TokenList();
    lx.tokenize(source.c_str(), *tk);
    bool ok = hasMeaningfulLogic(*tk);
    delete tk;
    return ok;
}

// --- JSON Helpers ---

std::string jsonEscape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

std::string getJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos);
    if (pos == std::string::npos) return "";
    size_t start = pos + 1;
    std::string val;
    bool esc = false;
    for (size_t i = start; i < json.size(); i++) {
        if (esc) {
            if (json[i] == 'n') val += '\n';
            else if (json[i] == 'r') val += '\r';
            else if (json[i] == 't') val += '\t';
            else if (json[i] == '"') val += '"';
            else if (json[i] == '\\') val += '\\';
            else val += json[i];
            esc = false;
        } else if (json[i] == '\\') {
            esc = true;
        } else if (json[i] == '"') {
            break;
        } else {
            val += json[i];
        }
    }
    return val;
}

int getJsonInt(const std::string& json, const std::string& key, int defVal = 0) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return defVal;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return defVal;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ' || json[pos] == '\t')) pos++;
    int val = 0;
    bool neg = false, has = false;
    if (pos < json.size() && json[pos] == '-') { neg = true; pos++; }
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + (json[pos] - '0');
        has = true;
        pos++;
    }
    if (!has) return defVal;
    return neg ? -val : val;
}

std::string urlDecode(const std::string& in) {
    std::string out;
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i] == '+') out += ' ';
        else if (in[i] == '%' && i + 2 < in.size()) {
            char hex[3] = { in[i+1], in[i+2], '\0' };
            int val = (int)strtol(hex, NULL, 16);
            out += (char)val;
            i += 2;
        } else out += in[i];
    }
    return out;
}

// --- HTTP Response Builders ---

void sendResponse(SOCKET s, const std::string& status, const std::string& contentType, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";
    oss << "Content-Type: " << contentType << "; charset=utf-8\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    std::string resp = oss.str();
    send(s, resp.c_str(), (int)resp.size(), 0);
}

// --- Request Router ---

void handleClient(SOCKET clientSock, StorageManager& store) {
    std::string request;
    char buffer[4096];
    int bytesRead = 0;

    // Read headers
    while ((bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        request += buffer;
        if (request.find("\r\n\r\n") != std::string::npos) break;
    }

    if (request.empty()) {
        closesocket(clientSock);
        return;
    }

    // Parse method and path
    std::istringstream iss(request);
    std::string method, url, proto;
    iss >> method >> url >> proto;

    if (method == "OPTIONS") {
        sendResponse(clientSock, "200 OK", "text/plain", "");
        closesocket(clientSock);
        return;
    }

    // Parse query string if present
    std::string path = url;
    std::string query = "";
    size_t qpos = url.find('?');
    if (qpos != std::string::npos) {
        path = url.substr(0, qpos);
        query = url.substr(qpos + 1);
    }

    // Read full body if Content-Length specified
    std::string body = "";
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headPart = request.substr(0, headerEnd);
        body = request.substr(headerEnd + 4);
        size_t clPos = headPart.find("Content-Length:");
        if (clPos == std::string::npos) clPos = headPart.find("content-length:");
        if (clPos != std::string::npos) {
            size_t clValStart = headPart.find(':', clPos) + 1;
            int cl = atoi(headPart.substr(clValStart).c_str());
            while ((int)body.size() < cl) {
                int r = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
                if (r <= 0) break;
                buffer[r] = '\0';
                body += buffer;
            }
        }
    }

    // --- Static File Routing ---
    if (method == "GET" && (path == "/" || path == "/index.html")) {
        std::string html;
        if (readFileToString("web/index.html", html)) {
            sendResponse(clientSock, "200 OK", "text/html", html);
        } else {
            sendResponse(clientSock, "404 Not Found", "text/plain", "index.html not found in web/");
        }
        closesocket(clientSock);
        return;
    }
    if (method == "GET" && path == "/style.css") {
        std::string css;
        if (readFileToString("web/style.css", css)) {
            sendResponse(clientSock, "200 OK", "text/css", css);
        } else {
            sendResponse(clientSock, "404 Not Found", "text/plain", "style.css not found");
        }
        closesocket(clientSock);
        return;
    }
    if (method == "GET" && path == "/app.js") {
        std::string js;
        if (readFileToString("web/app.js", js)) {
            sendResponse(clientSock, "200 OK", "application/javascript", js);
        } else {
            sendResponse(clientSock, "404 Not Found", "text/plain", "app.js not found");
        }
        closesocket(clientSock);
        return;
    }

    // --- API 1: Analyze & Save (Terminal Option 1) ---
    if (method == "POST" && path == "/api/analyze") {
        std::string project = getJsonString(body, "project");
        std::string code = getJsonString(body, "code");
        if (project.empty()) project = "UntitledProject";

        if (code.empty() || !prepare(code)) {
            std::string err = "{\"success\":false,\"error\":\"Incomplete or non-algorithmic input. No valid loop, recursion, or condition detected.\"}";
            sendResponse(clientSock, "200 OK", "application/json", err);
            closesocket(clientSock);
            return;
        }

        ComplexityResult r = analyzeSource(code);
        bool saved = store.saveSubmission(project, r.bigO, r.priorityRank, r.maxLoopDepth, r.recursionType, code);

        std::ostringstream json;
        json << "{"
             << "\"success\":true,"
             << "\"project\":\"" << jsonEscape(project) << "\","
             << "\"bigO\":\"" << jsonEscape(r.bigO) << "\","
             << "\"priorityRank\":" << r.priorityRank << ","
             << "\"loopCount\":" << r.loopCount << ","
             << "\"maxLoopDepth\":" << r.maxLoopDepth << ","
             << "\"hasDirectRecursion\":" << (r.hasDirectRecursion ? "true" : "false") << ","
             << "\"hasIndirectRecursion\":" << (r.hasIndirectRecursion ? "true" : "false") << ","
             << "\"recursionType\":\"" << jsonEscape(r.recursionType) << "\","
             << "\"saved\":" << (saved ? "true" : "false")
             << "}";
        sendResponse(clientSock, "200 OK", "application/json", json.str());
        closesocket(clientSock);
        return;
    }

    // --- API 2: Summary of Saved Projects (Terminal Option 2) ---
    if (method == "GET" && path == "/api/summary") {
        const int MAXP = 100;
        std::string* projects = new std::string[MAXP];
        int* counts = new int[MAXP];
        for (int i = 0; i < MAXP; i++) counts[i] = 0;
        int n = store.getUniqueProjects(projects, counts, MAXP);

        std::ostringstream json;
        json << "{\"projects\":[";
        for (int i = 0; i < n; i++) {
            SubmissionRecord last, opt;
            store.findLastByProject(projects[i].c_str(), last);
            store.findMostOptimalByProject(projects[i].c_str(), opt);
            if (i > 0) json << ",";
            json << "{"
                 << "\"name\":\"" << jsonEscape(projects[i]) << "\","
                 << "\"submissions\":" << counts[i] << ","
                 << "\"lastBigO\":\"" << jsonEscape(last.bigO) << "\","
                 << "\"optimalBigO\":\"" << jsonEscape(opt.bigO) << "\","
                 << "\"lastTimestamp\":\"" << jsonEscape(last.timestamp) << "\","
                 << "\"lastCode\":\"" << jsonEscape(last.sourceCode) << "\","
                 << "\"optimalCode\":\"" << jsonEscape(opt.sourceCode) << "\""
                 << "}";
        }
        json << "]}";
        delete[] projects;
        delete[] counts;

        sendResponse(clientSock, "200 OK", "application/json", json.str());
        closesocket(clientSock);
        return;
    }

    // --- API 3: Compare Two Projects (Terminal Option 3) ---
    if (method == "POST" && path == "/api/compare") {
        std::string p1 = getJsonString(body, "p1");
        std::string p2 = getJsonString(body, "p2");

        SubmissionRecord r1, r2;
        bool f1 = store.findMostOptimalByProject(p1.c_str(), r1);
        bool f2 = store.findMostOptimalByProject(p2.c_str(), r2);

        if (!f1 || !f2) {
            sendResponse(clientSock, "200 OK", "application/json", "{\"success\":false,\"error\":\"One or both projects not found.\"}");
            closesocket(clientSock);
            return;
        }

        std::string verdict;
        if (r1.priorityRank < r2.priorityRank) verdict = p1 + " is more optimal.";
        else if (r2.priorityRank < r1.priorityRank) verdict = p2 + " is more optimal.";
        else verdict = "Both have equivalent complexity rank.";

        std::ostringstream json;
        json << "{"
             << "\"success\":true,"
             << "\"p1\":{\"name\":\"" << jsonEscape(p1) << "\",\"bigO\":\"" << jsonEscape(r1.bigO) << "\",\"rank\":" << r1.priorityRank << "},"
             << "\"p2\":{\"name\":\"" << jsonEscape(p2) << "\",\"bigO\":\"" << jsonEscape(r2.bigO) << "\",\"rank\":" << r2.priorityRank << "},"
             << "\"verdict\":\"" << jsonEscape(verdict) << "\""
             << "}";
        sendResponse(clientSock, "200 OK", "application/json", json.str());
        closesocket(clientSock);
        return;
    }

    // --- API 4 & 5: View Last or Most Optimal Solution (Terminal Options 4 & 5) ---
    if (method == "GET" && path == "/api/project") {
        // Parse params: name and mode
        std::string pName = "";
        std::string mode = "last"; // "last" or "optimal"
        size_t namePos = query.find("name=");
        if (namePos != std::string::npos) {
            size_t amp = query.find('&', namePos);
            pName = urlDecode(query.substr(namePos + 5, amp == std::string::npos ? std::string::npos : amp - (namePos + 5)));
        }
        size_t modePos = query.find("mode=");
        if (modePos != std::string::npos) {
            size_t amp = query.find('&', modePos);
            mode = query.substr(modePos + 5, amp == std::string::npos ? std::string::npos : amp - (modePos + 5));
        }

        SubmissionRecord rec;
        bool found = false;
        if (mode == "optimal") {
            found = store.findMostOptimalByProject(pName.c_str(), rec);
        } else {
            found = store.findLastByProject(pName.c_str(), rec);
        }

        if (!found) {
            sendResponse(clientSock, "200 OK", "application/json", "{\"success\":false,\"error\":\"Project record not found.\"}");
            closesocket(clientSock);
            return;
        }

        std::ostringstream json;
        json << "{"
             << "\"success\":true,"
             << "\"project\":\"" << jsonEscape(rec.projectName) << "\","
             << "\"timestamp\":\"" << jsonEscape(rec.timestamp) << "\","
             << "\"bigO\":\"" << jsonEscape(rec.bigO) << "\","
             << "\"priorityRank\":" << rec.priorityRank << ","
             << "\"loopDepth\":" << rec.loopDepth << ","
             << "\"recursionType\":\"" << jsonEscape(rec.recursionType) << "\","
             << "\"sourceCode\":\"" << jsonEscape(rec.sourceCode) << "\""
             << "}";
        sendResponse(clientSock, "200 OK", "application/json", json.str());
        closesocket(clientSock);
        return;
    }

    // --- API 6: Run Demo Test Case (Terminal Option 6) ---
    if (method == "POST" && path == "/api/demo") {
        int c = getJsonInt(body, "demoId", 1);
        const char* paths[] = {
            "test_cases/1_python_nested.py",
            "test_cases/2_cpp_logarithmic.cpp",
            "test_cases/3_fib_exponential.cpp",
            "test_cases/4_indirect_recursion.txt"
        };
        const char* names[] = {
            "Demo_PythonNested",
            "Demo_CppLog",
            "Demo_Fibonacci",
            "Demo_Indirect"
        };

        if (c < 1 || c > 4) c = 1;
        std::string src;
        if (!readFileToString(paths[c-1], src) || !prepare(src)) {
            sendResponse(clientSock, "200 OK", "application/json", "{\"success\":false,\"error\":\"Unable to load demo test case file.\"}");
            closesocket(clientSock);
            return;
        }

        ComplexityResult r = analyzeSource(src);
        store.saveSubmission(names[c-1], r.bigO, r.priorityRank, r.maxLoopDepth, r.recursionType, src);

        std::ostringstream json;
        json << "{"
             << "\"success\":true,"
             << "\"demoName\":\"" << jsonEscape(names[c-1]) << "\","
             << "\"file\":\"" << jsonEscape(paths[c-1]) << "\","
             << "\"bigO\":\"" << jsonEscape(r.bigO) << "\","
             << "\"priorityRank\":" << r.priorityRank << ","
             << "\"loopCount\":" << r.loopCount << ","
             << "\"maxLoopDepth\":" << r.maxLoopDepth << ","
             << "\"recursionType\":\"" << jsonEscape(r.recursionType) << "\","
             << "\"sourceCode\":\"" << jsonEscape(src) << "\""
             << "}";
        sendResponse(clientSock, "200 OK", "application/json", json.str());
        closesocket(clientSock);
        return;
    }

    sendResponse(clientSock, "404 Not Found", "text/plain", "Not Found");
    closesocket(clientSock);
}

int main() {
    StorageManager store;

    // Initialize Winsock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket.\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    int selectedPort = 8080;
    bool bound = false;

    // Auto-scan for an open port starting from 8080 to 8100
    for (int port = 8080; port <= 8100; port++) {
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != SOCKET_ERROR) {
            selectedPort = port;
            bound = true;
            break;
        }
    }

    if (!bound) {
        std::cerr << "Could not bind to any port between 8080 and 8100.\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    if (listen(serverSock, 10) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }

    std::ostringstream urlOss;
    urlOss << "http://localhost:" << selectedPort;
    std::string appUrl = urlOss.str();

    std::cout << "========================================================\n";
    std::cout << "  CODELENS UNIVERSAL ANALYZER - WEB GUI RUNNING\n";
    std::cout << "========================================================\n";
    std::cout << "  URL: " << appUrl << "\n";
    std::cout << "  Using C++ Engine & StorageManager (submissions.txt)\n";
    std::cout << "  Press Ctrl+C in this terminal to stop the server.\n";
    std::cout << "========================================================\n\n";

    // Auto-launch the default browser
    std::string startCmd = "start " + appUrl;
    system(startCmd.c_str());

    while (true) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSock == INVALID_SOCKET) continue;
        handleClient(clientSock, store);
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}
