void mergeLike(int arr[], int n) {
    for (int i = 0; i < n; i++) {
        int j = n;
        while (j > 1) {
            j /= 2;
        }
        arr[i] = j;
    }
}
