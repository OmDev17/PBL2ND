#include <iostream>
#include <fstream>
#include <string>
using namespace std;

class FileManager
{
public:
    string files[3] = {
        "sorting alogs/bubblesort.txt",
        "sorting alogs/insertionsort.txt",
        "sorting alogs/selectionsort.txt"
    };

    string readComplexity(string filename);
    void printFile(string filename);
};