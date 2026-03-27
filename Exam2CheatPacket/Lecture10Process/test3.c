#include <stdio.h>
#include <unistd.h>

int main() {
    int a = fork();
    int b = fork();

    printf("PID=%d, PPID=%d, a=%d, b=%d\n", getpid(), getppid(), a, b);
    return 0;
}

