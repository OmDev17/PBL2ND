#include "../include/StorageManager.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <string>
#include<map>

const char* StorageManager::DB_FILE = "submissions.txt";

StorageManager::StorageManager() {}

void StorageManager::copyToChar(char* dest, const std::string& src, int maxLen) const {
    int i = 0;
    while (i < (int)src.size() && i < maxLen - 1) { dest[i] = src[i]; i++; }
    dest[i] = '\0';
}

// int StorageManager::strLen(const char* s) const {
//     int n = 0; while (s && s[n] != '\0') n++; return n;
// }

// bool StorageManager::strEqual(const char* a, const char* b) const {
//     if (!a || !b) return false;
//     int i = 0;
//     while (a[i] != '\0' && b[i] != '\0') { if (a[i] != b[i]) return false; i++; }
//     return a[i] == b[i];
// }

void StorageManager::escapeField(const std::string& in, char* out, int maxLen) const {
    int j = 0;
    for (int i = 0; i < (int)in.size() && j < maxLen - 2; i++) {
        if (in[i] == '|') { out[j++] = '\\'; out[j++] = 'p'; }
        else if (in[i] == '\n') { out[j++] = '\\'; out[j++] = 'n'; }
        else if (in[i] == '\r') { }
        else if (in[i] == '\\') { out[j++] = '\\'; out[j++] = '\\'; }
        else out[j++] = in[i];
    }
    out[j] = '\0';
}

// std::string StorageManager::unescapeField(const char* in) const {
//     std::string out;
//     if (!in) return out;
//     for (int i = 0; in[i] != '\0'; i++) {
//         if (in[i] == '\\' && in[i + 1] == 'p') { out += '|'; i++; }
//         else if (in[i] == '\\' && in[i + 1] == 'n') { out += '\n'; i++; }
//         else if (in[i] == '\\' && in[i + 1] == '\\') { out += '\\'; i++; }
//         else out += in[i];
//     }
//     return out;
// }

static int safeParseInt(const std::string& s, int defVal) {
    int val = 0; bool has = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') { val = val * 10 + (c - '0'); has = true; }
        else if (has) break;
    }
    return has ? val : defVal;
}

void StorageManager::makeTimestamp(char* out, int maxLen) const {
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);
    if (!now) { copyToChar(out, "unknown", maxLen); return; }
    char buf[32]; int p = 0;
    auto p2=[&](int v){ buf[p++]=char('0'+v/10); buf[p++]=char('0'+v%10); };
    auto p4=[&](int v){ buf[p++]=char('0'+(v/1000)%10); buf[p++]=char('0'+(v/100)%10); buf[p++]=char('0'+(v/10)%10); buf[p++]=char('0'+v%10); };
    p4(now->tm_year+1900); buf[p++]='-'; p2(now->tm_mon+1); buf[p++]='-'; p2(now->tm_mday);
    buf[p++]=' '; p2(now->tm_hour); buf[p++]=':'; p2(now->tm_min); buf[p++]=':'; p2(now->tm_sec);
    buf[p]='\0'; copyToChar(out, buf, maxLen);
}

bool StorageManager::saveSubmission(const std::string& project, const std::string& bigO,
                                    int rank, int loopDepth, const std::string& recType,
                                    const std::string& source) {
    std::ofstream out(DB_FILE, std::ios::app);
    if (!out.is_open()) return false;
    char ts[32], escSrc[MAX_SOURCE_LEN];
    makeTimestamp(ts, 32); escapeField(source, escSrc, MAX_SOURCE_LEN);
    out << project << "|" << ts << "|" << bigO << "|" << rank << "|"
        << loopDepth << "|" << recType << "|" << escSrc << "\n";
    out.close(); return true;
}

static bool splitFields(const std::string& line, std::string f[], int& count) {
    count = 0; size_t start = 0; size_t len = line.size();
    for (size_t i = 0; i < len && count < 7; i++) {
        if (line[i] == '|' && count < 6) {
            if (i < start) return false;
            f[count++] = line.substr(start, i - start);
            start = i + 1;
        }
    }
    if (start <= len) f[count++] = line.substr(start);
    return count >= 5;
}

static void fillRecord(const std::string f[], int fc, SubmissionRecord& rec) {
    // manual copy helpers via assignment to temp then copy
    auto cpy=[&](char* d,const std::string& s,int m){ int i=0; for(;i<(int)s.size()&&i<m-1;i++) d[i]=s[i]; d[i]='\0'; };
    cpy(rec.projectName, f[0], MAX_PROJECT_LEN);
    cpy(rec.timestamp,  f[1], 32);
    cpy(rec.bigO,       f[2], MAX_COMPLEXITY_LEN);
    rec.priorityRank = safeParseInt(f[3], 1);
    rec.loopDepth = 0; rec.recursionType[0] = '\0';
    int srcIdx = 4;
    if (fc >= 7) { rec.loopDepth = safeParseInt(f[4], 0); cpy(rec.recursionType, f[5], MAX_NAME_LEN); srcIdx = 6; }
    // unescape source
    std::string src;
    const std::string& in = f[srcIdx];
    for (size_t i = 0; i < in.size(); i++) {
        if (in[i]=='\\' && i+1<in.size() && in[i+1]=='p') { src+='|'; i++; }
        else if (in[i]=='\\' && i+1<in.size() && in[i+1]=='n') { src+='\n'; i++; }
        else if (in[i]=='\\' && i+1<in.size() && in[i+1]=='\\') { src+='\\'; i++; }
        else src += in[i];
    }
    cpy(rec.sourceCode, src, MAX_SOURCE_LEN);
}

// bool StorageManager::parseRecord(std::ifstream& in, SubmissionRecord& rec) const {
//     std::string line;
//     if (!std::getline(in, line)) return false;
//     if (line.empty() || line == "\r") return false;
//     std::string f[7]; int fc = 0;
//     if (!splitFields(line, f, fc)) return false;
//     fillRecord(f, fc, rec);
//     return rec.projectName[0] != '\0';
// }

// bool StorageManager::loadAll(SubmissionRecord records[], int& count) const {
//     count = 0;
//     std::ifstream in(DB_FILE);
//     if (!in.is_open()) return false;
//     while (count < MAX_RECORDS) {
//         SubmissionRecord r;
//         if (!parseRecord(in, r)) { if (in.eof()) break; continue; }
//         records[count++] = r;
//     }
//     in.close(); return true;
// }

bool StorageManager::findLastByProject(const char* project, SubmissionRecord& out) const {
    std::ifstream in(DB_FILE);
    if (!in.is_open()) return false;
    bool found = false; std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        std::string f[7]; int fc = 0;
        if (!splitFields(line, f, fc)) continue;
        if (f[0] == project) { fillRecord(f, fc, out); found = true; }
    }
    in.close(); return found;
}

bool StorageManager::findMostOptimalByProject(const char* project, SubmissionRecord& out) const {
    std::ifstream in(DB_FILE);
    if (!in.is_open()) return false;
    bool found = false; int best = 100000; std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;
        std::string f[7]; int fc = 0;
        if (!splitFields(line, f, fc)) continue;
        if (f[0] != project) continue;
        int rank = safeParseInt(f[3], 100000);
        if (!found || rank < best) { fillRecord(f, fc, out); best = rank; found = true; }
    }
    in.close(); return found;
}

// int StorageManager::getUniqueProjects(std::string projectList[], int subCounts[], int maxProjects) const {
//     std::ifstream in(DB_FILE);
//     if (!in.is_open()) return 0;
//     int unique = 0; std::string line;
//     while (std::getline(in, line)) {
//         if (line.empty() || line == "\r") continue;
//         std::string f[7]; int fc = 0;
//         if (!splitFields(line, f, fc)) continue;
//         int idx = -1;
//         for (int j = 0; j < unique; j++) if (projectList[j] == f[0]) { idx = j; break; }
//         if (idx == -1) {
//             if (unique >= maxProjects) continue;
//             projectList[unique] = f[0]; subCounts[unique] = 1; unique++;
//         } else subCounts[idx]++;
//     }
//     in.close(); return unique;
// }

int StorageManager::getUniqueProjects(std::string projectList[], int subCounts[], int maxProjects) const {
    std::ifstream in(DB_FILE);
    if (!in.is_open()) return 0;

    int unique = 0;
    std::string line;

    std::map<std::string, int> projectIndex;

    while (std::getline(in, line)) {
        if (line.empty() || line == "\r") continue;

        std::string f[7];
        int fc = 0;

        if (!splitFields(line, f, fc)) continue;

        std::map<std::string, int>::iterator it = projectIndex.find(f[0]);

        if (it == projectIndex.end()) {
            if (unique >= maxProjects) continue;

            projectList[unique] = f[0];
            subCounts[unique] = 1;

            projectIndex[f[0]] = unique;

            unique++;
        }
        else {
            subCounts[it->second]++;
        }
    }
    in.close();
    return unique;
}



std::string StorageManager::selectProjectInteractive() const {
    const int MAXP = 100;
    std::string* projects = new std::string[MAXP];
    int* counts = new int[MAXP];
    for (int i = 0; i < MAXP; i++) counts[i] = 0;
    int n = getUniqueProjects(projects, counts, MAXP);
    if (n == 0) {
        std::cout << "No saved projects found in submissions.txt yet!\n";
        delete[] projects; delete[] counts; return "";
    }
    std::cout << "--- Saved Projects in submissions.txt ---\n";
    for (int i = 0; i < n; i++) {
        std::cout << "  " << (i + 1) << ". " << projects[i] << " (" << counts[i]
                  << " submission" << (counts[i] != 1 ? "s" : "") << ")\n";
    }
    std::cout << "Select a project number (1-" << n << ", or 0 to cancel): ";
    std::string line; std::string result = "";
    if (std::getline(std::cin, line)) {
        int choice = safeParseInt(line, -1);
        if (choice > 0 && choice <= n) result = projects[choice - 1];
        else std::cout << "Selection cancelled/invalid.\n";
    }
    delete[] projects; delete[] counts;
    return result;
}

void StorageManager::printRecord(const SubmissionRecord& rec, bool showSource) const {
    std::cout << "  Project        : " << rec.projectName << "\n";
    std::cout << "  Timestamp      : " << rec.timestamp << "\n";
    std::cout << "  Big-O          : " << rec.bigO << "\n";
    std::cout << "  Priority Rank  : " << rec.priorityRank << "\n";
    std::cout << "  Loop Depth     : " << rec.loopDepth << "\n";
    std::cout << "  Recursion Type : " << (rec.recursionType[0] ? rec.recursionType : "None") << "\n";
    if (showSource) std::cout << "  Source Code:\n" << rec.sourceCode << "\n";
}
