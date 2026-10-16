#include <iostream>
#include "filemanager.h"
#include "prioritymanager.h"

using namespace std;

int main()
{
    FileManager fm;
    prioritymanager pm;

    int priorities[3] = {100, 100, 100};
    string complexities[3];
    int minpriority = 100;

    // Step 1: Read files and store priorities
    for (int i = 0; i < 3; i++)
    {
        complexities[i] = fm.readComplexity(fm.files[i]);
        if (!complexities[i].empty())
        {
            priorities[i] = pm.getpriority(complexities[i]);
            if (priorities[i] < minpriority)
                minpriority = priorities[i];
        }

        cout << fm.files[i] << " : "
             << (complexities[i].empty() ? "unavailable" : complexities[i])
             << " : Priority " << priorities[i] << endl;
    }

    if (minpriority == 100)
    {
        cout << "No valid complexity records found!" << endl;
        return 0;
    }

    // Step 2: Find minimum priority
    cout << "\nMinimum priority: " << minpriority << endl;

    // Step 3: Print files with minimum priority
    for (int i = 0; i < 3; i++)
    {
        if (!complexities[i].empty() && priorities[i] == minpriority)
        {
            cout << "\nBest Approach: "
                 << fm.files[i] << endl;

            cout << "----------------------\n";
            fm.printFile(fm.files[i]);
        }
    }

    return 0;
}