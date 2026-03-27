/*
Beginner Pipeline Notes + Code
=============================

This file focuses ONLY on pipelines:

    A | B
    A | B | C
    A | B | C | ...
    A | B | C > file

Core idea:
-----------
stdout of A → stdin of B → stdin of C → ...

We use:
- pipe()
- fork()
- dup2()
- exec()

Key rules:
-----------
1. Each connection between processes needs ONE pipe
2. For N commands, you need N-1 pipes
3. Always:
   - close unused file descriptors
   - setup dup2 BEFORE exec

Compile:
    gcc -Wall -Wextra -std=c11 pipeline_examples.c -o pipeline

Run:
    ./pipeline ab
    ./pipeline abc
    ./pipeline many
    ./pipeline redirect
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

void die(const char *msg) {
    perror(msg);
    exit(1);
}

/*
============================================================
1. A | B
============================================================
Example: ls | wc -l

Pipe count: 1
*/
void pipeline_ab() {
    int p[2];
    pipe(p);

    if (fork() == 0) {
        // A (ls)
        dup2(p[1], 1); // stdout -> pipe write
        close(p[0]);
        close(p[1]);
        execlp("ls", "ls", NULL);
        die("exec A");
    }

    if (fork() == 0) {
        // B (wc -l)
        dup2(p[0], 0); // stdin <- pipe read
        close(p[1]);
        close(p[0]);
        execlp("wc", "wc", "-l", NULL);
        die("exec B");
    }

    close(p[0]);
    close(p[1]);

    wait(NULL);
    wait(NULL);
}

/*
============================================================
2. A | B | C
============================================================
Example: ls | tr a-z A-Z | wc

Pipe count: 2
*/
void pipeline_abc() {
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);

    if (fork() == 0) {
        // A
        dup2(p1[1], 1);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("ls", "ls", NULL);
        die("exec A");
    }

    if (fork() == 0) {
        // B
        dup2(p1[0], 0);
        dup2(p2[1], 1);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("tr", "tr", "a-z", "A-Z", NULL);
        die("exec B");
    }

    if (fork() == 0) {
        // C
        dup2(p2[0], 0);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("wc", "wc", NULL);
        die("exec C");
    }

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);

    wait(NULL); wait(NULL); wait(NULL);
}

/*
============================================================
3. A | B | C | ... (general pattern)
============================================================

Key idea:
- loop through commands
- create pipes dynamically

This example runs:
    ls | grep .c | wc
*/
void pipeline_many() {
    char *cmds[][3] = {
        {"ls", NULL},
        {"grep", ".c", NULL},
        {"wc", NULL}
    };

    int n = 3;
    int prev_pipe[2];

    for (int i = 0; i < n; i++) {
        int curr_pipe[2];

        if (i < n - 1) {
            pipe(curr_pipe);
        }

        if (fork() == 0) {
            if (i > 0) {
                dup2(prev_pipe[0], 0);
            }

            if (i < n - 1) {
                dup2(curr_pipe[1], 1);
            }

            // close all pipes
            if (i > 0) {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }
            if (i < n - 1) {
                close(curr_pipe[0]);
                close(curr_pipe[1]);
            }

            execvp(cmds[i][0], cmds[i]);
            die("exec");
        }

        if (i > 0) {
            close(prev_pipe[0]);
            close(prev_pipe[1]);
        }

        if (i < n - 1) {
            prev_pipe[0] = curr_pipe[0];
            prev_pipe[1] = curr_pipe[1];
        }
    }

    for (int i = 0; i < n; i++) wait(NULL);
}

/*
============================================================
4. A | B | C > file
============================================================

Final process writes to file instead of screen
*/
void pipeline_redirect() {
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);

    if (fork() == 0) {
        dup2(p1[1], 1);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("ls", "ls", NULL);
        die("exec A");
    }

    if (fork() == 0) {
        dup2(p1[0], 0);
        dup2(p2[1], 1);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        execlp("grep", "grep", ".c", NULL);
        die("exec B");
    }

    if (fork() == 0) {
        int fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(p2[0], 0);
        dup2(fd, 1);

        close(fd);
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);

        execlp("wc", "wc", NULL);
        die("exec C");
    }

    close(p1[0]); close(p1[1]);
    close(p2[0]); close(p2[1]);

    wait(NULL); wait(NULL); wait(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s ab|abc|many|redirect\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "ab") == 0) {
        pipeline_ab();
    } else if (strcmp(argv[1], "abc") == 0) {
        pipeline_abc();
    } else if (strcmp(argv[1], "many") == 0) {
        pipeline_many();
    } else if (strcmp(argv[1], "redirect") == 0) {
        pipeline_redirect();
    } else {
        printf("Invalid option\n");
        return 1;
    }

    return 0;
}
