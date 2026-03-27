/*
Beginner Notes: wait(), waitpid(), and Exit Status Macros
=========================================================

This file explains how to check what happened to a child process
using wait() and waitpid().

Topics covered:
    exit()
    wait()
    waitpid()
    WIFEXITED
    WEXITSTATUS
    WIFSIGNALED
    WTERMSIG
    WIFSTOPPED
    WSTOPSIG

------------------------------------------------------------
1. Big idea: what is exit status?
------------------------------------------------------------
When a child process finishes, it returns a number (exit code).

Example:
    exit(5);

The parent can read this using wait() or waitpid().

------------------------------------------------------------
2. wait() basics
------------------------------------------------------------
int status;
pid_t pid = wait(&status);

status is NOT the exit code directly.
You MUST use macros to interpret it.

------------------------------------------------------------
3. Key macros (VERY IMPORTANT FOR EXAMS)
------------------------------------------------------------

WIFEXITED(status)
    -> returns true if child exited normally (using exit)

WEXITSTATUS(status)
    -> gives the exit code (only if WIFEXITED is true)

WIFSIGNALED(status)
    -> true if child was killed by a signal

WTERMSIG(status)
    -> gives which signal killed the child

WIFSTOPPED(status)
    -> true if child was stopped (rare in basic classes)

WSTOPSIG(status)
    -> gives stop signal

------------------------------------------------------------
4. Compile
------------------------------------------------------------
    gcc -Wall -Wextra -std=c11 wait_status_notes_examples.c -o wait_notes

Run:
    ./wait_notes normal
    ./wait_notes signal
    ./wait_notes multiple
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

/*
Helper: print status in a beginner-friendly way
*/
void explain_status(int status) {
    if (WIFEXITED(status)) {
        printf("Child exited normally\n");
        printf("Exit code = %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status)) {
        printf("Child was killed by a signal\n");
        printf("Signal number = %d\n", WTERMSIG(status));
    }
    else if (WIFSTOPPED(status)) {
        printf("Child was stopped\n");
        printf("Stop signal = %d\n", WSTOPSIG(status));
    }
    else {
        printf("Other status change\n");
    }
}

/*
============================================================
1. Normal exit
============================================================
Child calls exit(7)
Parent reads status
*/
void demo_normal() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[child] exiting with code 7\n");
        exit(7);
    }

    int status;
    wait(&status);

    printf("[parent] checking child status:\n");
    explain_status(status);
}

/*
============================================================
2. Child killed by signal
============================================================
We send SIGKILL to child
*/
void demo_signal() {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[child] going to sleep\n");
        sleep(10);
        exit(0);
    }

    sleep(1); // give child time to start
    kill(pid, SIGKILL);

    int status;
    wait(&status);

    printf("[parent] checking child status:\n");
    explain_status(status);
}

/*
============================================================
3. Multiple children
============================================================
Parent collects all children
*/
void demo_multiple() {
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            printf("[child %d] exiting with %d\n", i, i + 1);
            exit(i + 1);
        }
    }

    for (int i = 0; i < 3; i++) {
        int status;
        pid_t ended = wait(&status);

        printf("[parent] child pid %d finished\n", ended);
        explain_status(status);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s normal|signal|multiple\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "normal") == 0) {
        demo_normal();
    }
    else if (strcmp(argv[1], "signal") == 0) {
        demo_signal();
    }
    else if (strcmp(argv[1], "multiple") == 0) {
        demo_multiple();
    }
    else {
        printf("Invalid option\n");
        return 1;
    }

    return 0;
}
