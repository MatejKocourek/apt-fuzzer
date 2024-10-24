#include <stdio.h>
#include <stdlib.h>

long long fibonacci(int n) {
    if (n <= 1) return n;
    long long a = 0, b = 1, c;
    for (int i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
    }
    return b;
}

int main(int argc, char* argv[]) {
    if(argc<2)
        return 1;
    const int N = atoi(argv[1]);
    
    long long result = fibonacci(N);

    // Output the result and the time taken
    printf("Fibonacci(%d) = %lld\n", N, result);

    return 0;
}