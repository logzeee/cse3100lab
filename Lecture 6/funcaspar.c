#include <stdio.h>

// Function that adds two numbers
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

// Function that takes a function as a parameter
// and applies it to two numbers

void applyOperation(int (*operation)(int, int), int x, int y) {
    int result = operation(x, y);
    printf("Result: %d\n", result);
}

int main() {
    applyOperation(add, 5, 3);
    applyOperation(subtract, 5, 3);
    return 0;
}

