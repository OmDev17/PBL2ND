#include "filemanager.h"
#include <stdexcept>

string FileManager::readComplexity(string filename)
{
    ifstream fin(filename);
    // Exception handling is used to handle file-opening errors safely.
    // The try block checks whether the file opened successfully.
    // If the file cannot be opened, a runtime_error exception is thrown.
    // The catch block displays the error message and returns an empty string
    // so that the calling code can handle the failure.
    try
    {
        if (!fin.is_open())
        {
            throw runtime_error("File could not be opened: " + filename);
        }

        string line, complexity = "";

        while (getline(fin, line))
        {
            string marker = "Time Complexity:";
            size_t markerPosition = line.find(marker);
            if (markerPosition == string::npos)
            {
                marker = "Time complexity:";
                markerPosition = line.find(marker);
            }

            if (markerPosition != string::npos)
            {
                size_t valuePosition = line.find(':', markerPosition) + 1;
                while (valuePosition < line.size() &&
                       (line[valuePosition] == ' ' || line[valuePosition] == '\t'))
                {
                    valuePosition++;
                }
                complexity = line.substr(valuePosition);
            }
        }

        fin.close();
        return complexity;
    }
    catch (const exception& e)
    {
        cout << "Error: " << e.what() << endl;
        return "";
    }
}

void FileManager::printFile(string filename)
{
    ifstream fin(filename);

    // Exception handling is used to handle file-opening errors.
    // If the file cannot be opened, a runtime_error is thrown.
    // The catch block displays the error message.

    try
    {
        if (!fin.is_open())
        {
            throw runtime_error("File could not be opened: " + filename);
        }

        string line;

        while (getline(fin, line))
        {
            cout << line << endl;
        }

        fin.close();
    }
    catch (const exception& e)
    {
        cout << "Error: " << e.what() << endl;
    }
}