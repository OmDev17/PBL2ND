#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "DataModels.h"

// Flat-file persistence and query operations for submissions.
class StorageManager {
private:
    static const char* DB_FILE;
    void copyToChar(char* dest, const std::string& src, int maxLen) const;
    void escapeField(const std::string& in, char* out, int maxLen) const;
    std::string unescapeField(const char* in) const;
    bool strEqual(const char* a, const char* b) const;
    int strLen(const char* s) const;
    void makeTimestamp(char* out, int maxLen) const;
    bool parseRecord(std::ifstream& in, SubmissionRecord& rec) const;

public:
    StorageManager();
    bool saveSubmission(const std::string& project, const std::string& bigO,
                        int rank, int loopDepth, const std::string& recType,
                        const std::string& source);
    bool loadAll(SubmissionRecord records[], int& count) const;
    bool findLastByProject(const char* project, SubmissionRecord& out) const;
    bool findMostOptimalByProject(const char* project, SubmissionRecord& out) const;
    int getUniqueProjects(std::string projectList[], int subCounts[], int maxProjects) const;
    std::string selectProjectInteractive() const;
    void printRecord(const SubmissionRecord& rec, bool showSource) const;
};

#endif // STORAGE_MANAGER_H
