/*
Beginner Notes: Pipes in C
==========================

This file is meant to be READ like notes and also used as real code.
It explains the most important pipe scenarios you are likely to see in class.

Big idea of a pipe
------------------
A pipe is a one-way communication channel between processes.
You can think of it like this:

    write end  ----->  kernel buffer  ----->  read end

In C on Unix/Linux, a pipe is created with:

    int pipe(int pipefd[2]);

After calling pipe(pipefd):
    pipefd[0] = read end
    pipefd[1] = write end

Very important rule:
    write to pipefd[1]
    read  from pipefd[0]

Common beginner idea
--------------------
A pipe by itself does not make communication happen.
Usually you do this pattern:
    1. create pipe
    2. fork()
    3. close unused ends in each process
    4. write() in one process
    5. read() in the other process

Why close unused ends?
----------------------
Because if a process keeps an end open that it does not use, the other side
may wait forever for more data or EOF.

For example, if parent reads and child writes:
    parent should close(pipefd[1]);
    child  should close(pipefd[0]);

Mental model for parameters
---------------------------
1. pipe(int pipefd[2])
   - pipefd is an array of 2 ints
   - after success:
       pipefd[0] = read end
       pipefd[1] = write end
   - returns 0 on success, -1 on error

2. read(fd, buf, nbytes)
   - fd     = where to read from
   - buf    = where data should go in memory
   - nbytes = max number of bytes to read
   - returns number of bytes actually read
   - returns 0 for EOF
   - returns -1 on error

3. write(fd, buf, nbytes)
   - fd     = where to write to
   - buf    = bytes to send
   - nbytes = number of bytes to write
   - returns number of bytes actually written
   - returns -1 on error

4. dup2(oldfd, newfd)
   - makes newfd refer to the same open file as oldfd
   - very useful for redirecting stdin/stdout before exec

How to compile
--------------
    gcc -Wall -Wextra -std=c11 pipe_family_notes_examples.c -o pipe_notes

How to run
----------
    ./pipe_notes basic
    ./pipe_notes string
    ./pipe_notes parent_to_child
    ./pipe_notes child_to_parent
    ./pipe_notes two_pipes
    ./pipe_notes pipeline
    ./pipe_notes partial

What this file covers
---------------------
1. basic pipe creation
2. parent writes, child reads
3. child writes, parent reads
4. sending an int through a pipe
5. sending a string through a pipe
6. two-way communication using two pipes
7. using pipe + fork + dup2 + exec for A | B
8. partial read/write loop pattern
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define RD 0
#define WR 1

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void wait_for_child(pid_t pid) {
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        die("waitpid");
    }

    if (WIFEXITED(status)) {
        printf("[parent] child exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[parent] child killed by signal %d\n", WTERMSIG(status));
    }
}

/*
Helper: write exactly n bytes.
Why this matters:
write() may write fewer bytes than requested, so loops are the safe pattern.
For small classroom examples, one write often works, but this helper shows the
more standard robust way to think.
*/
static void write_all(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    while (count > 0) {
        ssize_t nw = write(fd, p, count);
        if (nw < 0) {
            die("write");
        }
        p += nw;
        count -= (size_t)nw;
    }
}

/*
Helper: read exactly n bytes unless EOF happens too early.
Useful when sending fixed-size data like one int.
*/
static void read_exact(int fd, void *buf, size_t count) {
    char *p = (char *)buf;
    while (count > 0) {
        ssize_t nr = read(fd, p, count);
        if (nr < 0) {
            die("read");
        }
        if (nr == 0) {
            fprintf(stderr, "Unexpected EOF while reading exact number of bytes\n");
            exit(EXIT_FAILURE);
        }
        p += nr;
        count -= (size_t)nr;
    }
}

/*
============================================================
1. Basic pipe creation
============================================================
When to use:
Use this when you are first learning what pipe() returns and how to store the
read and write ends.

What this example does:
Creates a pipe and prints the two file descriptors.
No fork, no communication yet.

Why this is useful:
It builds the basic mental model:
    p[0] = read end
    p[1] = write end
*/
static void demo_basic(void) {
    int p[2];

    if (pipe(p) == -1) {
        die("pipe");
    }

    printf("Pipe created successfully\n");
    printf("p[0] = %d (read end)\n", p[0]);
    printf("p[1] = %d (write end)\n", p[1]);

    close(p[0]);
    close(p[1]);
}

/*
============================================================
2. Parent writes an int, child reads an int
============================================================
When to use:
Use this as the standard first real pipe pattern.
It shows one-way communication from parent to child.

What this example does:
- parent creates pipe
- parent forks
- parent writes an int into the pipe
- child reads the int from the pipe

Standard pattern to think about:
If parent writes and child reads:
    parent closes read end
    child closes write end
*/
static void demo_parent_to_child(void) {
    int p[2];
    pid_t pid;

    if (pipe(p) == -1) {
        die("pipe");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int value = 0;

        close(p[WR]);
        read_exact(p[RD], &value, sizeof(value));
        close(p[RD]);

        printf("[child] received int: %d\n", value);
        exit(0);
    }

    close(p[RD]);
    {
        int value = 42;
        write_all(p[WR], &value, sizeof(value));
        close(p[WR]);
        printf("[parent] sent int: %d\n", value);
    }

    wait_for_child(pid);
}

/*
============================================================
3. Child writes an int, parent reads an int
============================================================
When to use:
Use this when a question asks the child to compute something and send the
result back to the parent.

What this example does:
- parent creates pipe
- child computes a value
- child writes to the pipe
- parent reads from the pipe

This is one of the most common exam patterns.
*/
static void demo_child_to_parent(void) {
    int p[2];
    pid_t pid;

    if (pipe(p) == -1) {
        die("pipe");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int result = 7 + 8 + 9;

        close(p[RD]);
        write_all(p[WR], &result, sizeof(result));
        close(p[WR]);

        printf("[child] sent result: %d\n", result);
        exit(0);
    }

    close(p[WR]);
    {
        int result = 0;
        read_exact(p[RD], &result, sizeof(result));
        close(p[RD]);

        printf("[parent] received result: %d\n", result);
    }

    wait_for_child(pid);
}

/*
============================================================
4. Sending a string through a pipe
============================================================
When to use:
Use this when the problem involves messages or text instead of ints.

What this example does:
- parent writes a string to the pipe
- child reads bytes from the pipe into a buffer
- child prints the message

Important beginner note:
Strings are just bytes.
Usually we include the '\0' terminator if we want the receiver to treat the
received data as a C string directly.
*/
static void demo_string(void) {
    int p[2];
    pid_t pid;

    if (pipe(p) == -1) {
        die("pipe");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        char buf[100];
        ssize_t nr;

        close(p[WR]);
        nr = read(p[RD], buf, sizeof(buf));
        if (nr < 0) {
            die("read");
        }
        close(p[RD]);

        printf("[child] read %zd bytes\n", nr);
        printf("[child] message: %s\n", buf);
        exit(0);
    }

    close(p[RD]);
    {
        const char msg[] = "hello through a pipe";
        write_all(p[WR], msg, sizeof(msg));
        close(p[WR]);
        printf("[parent] sent message: %s\n", msg);
    }

    wait_for_child(pid);
}

/*
============================================================
5. Two-way communication: parent sends n, child sends answer back
============================================================
When to use:
Use this when communication must go both directions.
One pipe is only one-way, so for two-way communication you usually need
TWO pipes.

What this example does:
- p_to_c sends data from parent to child
- c_to_p sends data from child to parent
- parent sends n
- child computes 1 + 2 + ... + n
- child sends the sum back

Very important beginner rule:
For two-way communication, use two pipes.
*/
static void demo_two_pipes(void) {
    int p_to_c[2];
    int c_to_p[2];
    pid_t pid;

    if (pipe(p_to_c) == -1) {
        die("pipe p_to_c");
    }
    if (pipe(c_to_p) == -1) {
        die("pipe c_to_p");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int n = 0;
        int sum = 0;
        int i;

        close(p_to_c[WR]);
        close(c_to_p[RD]);

        read_exact(p_to_c[RD], &n, sizeof(n));
        close(p_to_c[RD]);

        for (i = 1; i <= n; i++) {
            sum += i;
        }

        write_all(c_to_p[WR], &sum, sizeof(sum));
        close(c_to_p[WR]);

        printf("[child] got n=%d and sent sum=%d\n", n, sum);
        exit(0);
    }

    close(p_to_c[RD]);
    close(c_to_p[WR]);

    {
        int n = 10;
        int sum = 0;

        write_all(p_to_c[WR], &n, sizeof(n));
        close(p_to_c[WR]);
        printf("[parent] sent n=%d\n", n);

        read_exact(c_to_p[RD], &sum, sizeof(sum));
        close(c_to_p[RD]);
        printf("[parent] received sum=%d\n", sum);
    }

    wait_for_child(pid);
}

/*
============================================================
6. Classic pipeline: A | B using pipe + fork + dup2 + exec
============================================================
When to use:
Use this when the question is about making one program's stdout become another
program's stdin, like shell pipelines.

What this example does:
Builds the equivalent of:
    ls | wc -l

How to think about the pattern:
1. create pipe
2. fork child A
   - redirect stdout to pipe write end
   - exec A
3. fork child B
   - redirect stdin to pipe read end
   - exec B
4. parent closes both ends and waits

Important note:
Programs A and B do not know they are in a pipeline.
They just use stdout and stdin.
*/
static void demo_pipeline(void) {
    int p[2];
    pid_t pid_a;
    pid_t pid_b;

    if (pipe(p) == -1) {
        die("pipe");
    }

    pid_a = fork();
    if (pid_a < 0) {
        die("fork for A");
    }

    if (pid_a == 0) {
        close(p[RD]);
        if (dup2(p[WR], STDOUT_FILENO) == -1) {
            die("dup2 A");
        }
        close(p[WR]);

        execlp("ls", "ls", (char *)NULL);
        die("execlp ls");
    }

    pid_b = fork();
    if (pid_b < 0) {
        die("fork for B");
    }

    if (pid_b == 0) {
        close(p[WR]);
        if (dup2(p[RD], STDIN_FILENO) == -1) {
            die("dup2 B");
        }
        close(p[RD]);

        execlp("wc", "wc", "-l", (char *)NULL);
        die("execlp wc");
    }

    close(p[RD]);
    close(p[WR]);

    wait_for_child(pid_a);
    wait_for_child(pid_b);
}

/*
============================================================
7. Partial read/write loop example
============================================================
When to use:
Use this when the problem involves a lot of bytes or when you want the safe,
standard pattern for repeated reads until EOF.

What this example does:
- parent writes a longer message in one direction
- child reads in smaller chunks in a loop until EOF

Key thinking pattern:
read() can return:
    > 0 : some bytes were read
      0 : EOF
    -1 : error

So a common loop is:
    while ((n = read(...)) > 0) { ... }
*/
static void demo_partial(void) {
    int p[2];
    pid_t pid;

    if (pipe(p) == -1) {
        die("pipe");
    }

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        char buf[8];
        ssize_t nr;

        close(p[WR]);
        printf("[child] reading in small chunks:\n");

        while ((nr = read(p[RD], buf, sizeof(buf) - 1)) > 0) {
            buf[nr] = '\0';
            printf("[child] chunk (%zd bytes): \"%s\"\n", nr, buf);
        }

        if (nr < 0) {
            die("read");
        }

        close(p[RD]);
        exit(0);
    }

    close(p[RD]);
    {
        const char *msg = "This is a longer message to show repeated read calls.";
        write_all(p[WR], msg, strlen(msg));
        close(p[WR]);
        printf("[parent] sent long message\n");
    }

    wait_for_child(pid);
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s MODE\n\n"
        "Modes:\n"
        "  basic            - create a pipe and print its fds\n"
        "  string           - parent sends a string to child\n"
        "  parent_to_child  - parent sends an int to child\n"
        "  child_to_parent  - child sends an int to parent\n"
        "  two_pipes        - two-way communication using two pipes\n"
        "  pipeline         - build ls | wc -l using pipe + exec\n"
        "  partial          - repeated read loop until EOF\n\n"
        "Examples:\n"
        "  %s basic\n"
        "  %s string\n"
        "  %s parent_to_child\n"
        "  %s child_to_parent\n"
        "  %s two_pipes\n"
        "  %s pipeline\n"
        "  %s partial\n",
        prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "basic") == 0) {
        demo_basic();
    } else if (strcmp(argv[1], "string") == 0) {
        demo_string();
    } else if (strcmp(argv[1], "parent_to_child") == 0) {
        demo_parent_to_child();
    } else if (strcmp(argv[1], "child_to_parent") == 0) {
        demo_child_to_parent();
    } else if (strcmp(argv[1], "two_pipes") == 0) {
        demo_two_pipes();
    } else if (strcmp(argv[1], "pipeline") == 0) {
        demo_pipeline();
    } else if (strcmp(argv[1], "partial") == 0) {
        demo_partial();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
