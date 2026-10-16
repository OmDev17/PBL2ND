#include "prioritymanager.h"

int prioritymanager::getpriority(string c)
{
    if (c == "O(1)") return 1;
    else if (c == "O(log n)") return 2;
    else if (c == "O(n)") return 3;
    else if (c == "O(n log n)") return 4;
    else if (c == "O(n^2)" || c == "O(n\xC2\xB2)") return 5;
    else if (c == "O(n^3)") return 6;
    else if (c == "O(2^n)") return 7;
    else if (c == "O(n!)") return 8;

    return 100;
}