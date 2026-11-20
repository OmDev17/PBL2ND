#include <iostream>

int binaryLikeSteps(int n) {
    int count = 0;
    int i = 1;
    while (i < n) {
        i *= 2;
        count++;
    }
    return count;
}
