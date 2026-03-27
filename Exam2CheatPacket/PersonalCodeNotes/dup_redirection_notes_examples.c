/*
Beginner Notes: dup(), dup2(), and Redirection in C
===================================================

This file is meant to be READ like notes and also used as real code.
It focuses on:

    Shell redirection ideas
    dup()
    dup2()
    redirecting stdout
    redirecting stdin
    redirecting stderr
    restoring stdout after redirection
    redirection before exec()
    child-process redirection patterns

This is written for a BEGINNER.
The large comments are the notes.
The functions are the examples.

------------------------------------------------------------
1. Big idea: what is redirection?
------------------------------------------------------------
In the shell, you may see:

    command > out.txt
    command < in.txt
    command 2> err.txt
    command >> out.txt

These mean:
    >   redirect stdout to a file
    <   redirect stdin from a file
    2>  redirect stderr to a file
    >>  append stdout to a file

Important beginner idea:
The program itself usually still reads from stdin and writes to stdout/stderr.
What changes is WHICH file descriptor those names point to.

------------------------------------------------------------
2. File descriptors you must know
------------------------------------------------------------
    0 = stdin
    1 = stdout
    2 = stderr

So redirection is mostly about making:
    0 point somewhere else
    1 point somewhere else
    2 point somewhere else

------------------------------------------------------------
3. How dup() works
------------------------------------------------------------
Prototype:
    int dup(int oldfd);

How to think about parameter:
    oldfd = an already-open file descriptor you want to copy

What dup() does:
    It creates a NEW file descriptor that refers to the SAME open file.
    The new fd is the lowest available descriptor number.

Returns:
    new file descriptor on success
    -1 on error

Important idea:
If fd1 and fd2 are duplicates, they share the same open file description,
which means they share the file position.

------------------------------------------------------------
4. How dup2() works
------------------------------------------------------------
Prototype:
    int dup2(int oldfd, int newfd);

How to think about parameters:
    oldfd = already-open file descriptor
    newfd = exact fd number you want to replace/use

What dup2() does:
    It makes newfd refer to the same open file as oldfd.
    If newfd was already open, dup2() closes it first.

Returns:
    newfd on success
    -1 on error

This is the redirection superstar.
Why?
Because you can do things like:
    dup2(fd, 1);   // stdout now goes to fd's file
    dup2(fd, 0);   // stdin now comes from fd's file
    dup2(fd, 2);   // stderr now goes to fd's file

------------------------------------------------------------
5. Beginner way to picture redirection
------------------------------------------------------------
Normally:
    fd 0 -> keyboard/input
    fd 1 -> terminal screen
    fd 2 -> terminal error screen

After redirecting stdout to a file:
    fd 1 -> output.txt

Then printf() writes to output.txt instead of the terminal.

------------------------------------------------------------
6. Shell idea vs C code idea
------------------------------------------------------------
Shell command:
    ls > out.txt

C idea behind it:
    open out.txt for writing
    dup2(file_fd, STDOUT_FILENO)
    close(file_fd)
    exec ls

------------------------------------------------------------
7. Compile and run
------------------------------------------------------------
Compile:
    gcc -Wall -Wextra -std=c11 dup_redirection_notes_examples.c -o dup_notes

Run examples:
    ./dup_notes dup_basic
    ./dup_notes dup_shared_offset
    ./dup_notes stdout_file
    ./dup_notes stderr_file
    ./dup_notes stdin_file
    ./dup_notes restore_stdout
    ./dup_notes exec_redirect_out
    ./dup_notes exec_redirect_in
    ./dup_notes exec_redirect_err
    ./dup_notes shell_like

------------------------------------------------------------
8. Important beginner warnings
------------------------------------------------------------
- Always check return values
- Always close file descriptors you do not need
- After dup2(fd, 1), stdout goes to the same place as fd
- After exec(), open file descriptors stay open unless you changed flags
- In child-process redirection, setup dup2 BEFORE exec()
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

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
============================================================
1. Basic dup()
============================================================
When to use:
Use dup() when you want another descriptor referring to the same open file.
This is the simplest introduction to descriptor duplication.

What this example does:
- opens a file for writing
- duplicates the descriptor with dup()
- writes using both fds

What to learn:
Both descriptors refer to the same open file.
*/
static void demo_dup_basic(void) {
    int fd1 = open("dup_basic.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int fd2;

    if (fd1 < 0) {
        die("open dup_basic");
    }

    fd2 = dup(fd1);
    if (fd2 < 0) {
        close(fd1);
        die("dup");
    }

    write_all(fd1, "Written via fd1\n", 16);
    write_all(fd2, "Written via fd2\n", 16);

    printf("fd1 = %d, fd2 = %d\n", fd1, fd2);

    close(fd1);
    close(fd2);
}

/*
============================================================
2. dup() and shared file position
============================================================
When to use:
Use this to understand that duplicated descriptors share file position.
This is a very important concept.

What this example does:
- opens a file
- dup() makes a second fd
- writes using fd1, then fd2
- because they share the same position, the second write continues after first
*/
static void demo_dup_shared_offset(void) {
    int fd1 = open("dup_offset.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int fd2;

    if (fd1 < 0) {
        die("open dup_offset");
    }

    fd2 = dup(fd1);
    if (fd2 < 0) {
        close(fd1);
        die("dup");
    }

    write_all(fd1, "ABC", 3);
    write_all(fd2, "DEF", 3);

    close(fd1);
    close(fd2);

    printf("dup_offset.txt should now contain: ABCDEF\n");
}

/*
============================================================
3. Redirect stdout to a file with dup2()
============================================================
When to use:
Use this when you want normal output like printf() to go into a file.
This is the most important redirection pattern.

What this example does:
- opens stdout_file.txt
- uses dup2(fd, 1)
- then printf() writes into the file

Standard pattern:
    fd = open(...)
    dup2(fd, STDOUT_FILENO)
    close(fd)
    printf(...)
*/
static void demo_stdout_file(void) {
    int fd = open("stdout_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        die("open stdout_file");
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        close(fd);
        die("dup2 stdout");
    }

    close(fd);

    printf("This line goes into stdout_file.txt\n");
    printf("So does this one\n");
    fflush(stdout);
}

/*
============================================================
4. Redirect stderr to a file with dup2()
============================================================
When to use:
Use this when you want error messages to go to a file,
like shell redirection using 2>.

What this example does:
- opens stderr_file.txt
- redirects fd 2 there
- writes an error-style message
*/
static void demo_stderr_file(void) {
    int fd = open("stderr_file.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        die("open stderr_file");
    }

    if (dup2(fd, STDERR_FILENO) < 0) {
        close(fd);
        die("dup2 stderr");
    }

    close(fd);

    dprintf(STDERR_FILENO, "This error message goes into stderr_file.txt\n");
}

/*
============================================================
5. Redirect stdin from a file with dup2()
============================================================
When to use:
Use this when you want a program to read input from a file instead of keyboard.
This is the C version of shell input redirection using <.

What this example does:
- creates input_redir.txt
- reopens it for reading
- dup2(fd, 0) makes stdin come from that file
- read from stdin and print result

Important beginner idea:
stdin is just fd 0.
*/
static void demo_stdin_file(void) {
    int fd;
    char buf[100];
    ssize_t nr;

    fd = open("input_redir.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        die("open input_redir for write");
    }
    write_all(fd, "Hello from redirected stdin\n", 28);
    close(fd);

    fd = open("input_redir.txt", O_RDONLY);
    if (fd < 0) {
        die("open input_redir for read");
    }

    if (dup2(fd, STDIN_FILENO) < 0) {
        close(fd);
        die("dup2 stdin");
    }

    close(fd);

    nr = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (nr < 0) {
        die("read redirected stdin");
    }

    buf[nr] = '\0';
    printf("Read from redirected stdin: %s", buf);
}

/*
============================================================
6. Save and restore stdout
============================================================
When to use:
Use this when you want to temporarily redirect stdout,
then later restore it back to the terminal.

What this example does:
- saves current stdout with dup(1)
- redirects stdout to a file
- prints into the file
- restores old stdout
- prints to terminal again

This is a very useful pattern.
*/
static void demo_restore_stdout(void) {
    int saved_stdout;
    int fd;

    saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        die("dup save stdout");
    }

    fd = open("temporary_redirect.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        close(saved_stdout);
        die("open temporary_redirect");
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        close(fd);
        close(saved_stdout);
        die("dup2 temporary stdout");
    }
    close(fd);

    printf("This goes into temporary_redirect.txt\n");
    fflush(stdout);

    if (dup2(saved_stdout, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        die("restore stdout");
    }
    close(saved_stdout);

    printf("This goes back to the terminal\n");
}

/*
============================================================
7. Redirect child stdout before exec()
============================================================
When to use:
Use this for shell-like questions such as command > file.
This is one of the most important practical patterns.

What this example does:
- parent forks
- child opens child_out.txt
- child redirects stdout to that file
- child execs ls
- parent waits

Shell idea:
    ls > child_out.txt
*/
static void demo_exec_redirect_out(void) {
    pid_t pid = fork();

    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int fd = open("child_out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            die("open child_out");
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            close(fd);
            die("dup2 child stdout");
        }
        close(fd);

        execlp("ls", "ls", NULL);
        die("execlp ls");
    }

    wait(NULL);
    printf("Finished running ls with stdout redirected to child_out.txt\n");
}

/*
============================================================
8. Redirect child stdin before exec()
============================================================
When to use:
Use this for shell-like questions such as command < file.

What this example does:
- creates child_in.txt with text in it
- parent forks
- child redirects stdin from child_in.txt
- child execs wc -w

Shell idea:
    wc -w < child_in.txt
*/
static void demo_exec_redirect_in(void) {
    int fd = open("child_in.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    pid_t pid;

    if (fd < 0) {
        die("open child_in for write");
    }
    write_all(fd, "one two three four five\n", 24);
    close(fd);

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        fd = open("child_in.txt", O_RDONLY);
        if (fd < 0) {
            die("open child_in for read");
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            close(fd);
            die("dup2 child stdin");
        }
        close(fd);

        execlp("wc", "wc", "-w", NULL);
        die("execlp wc");
    }

    wait(NULL);
}

/*
============================================================
9. Redirect child stderr before exec()
============================================================
When to use:
Use this for shell-like questions such as command 2> file.

What this example does:
- parent forks
- child redirects stderr to child_err.txt
- child runs ls on a file that should not exist
- the error message goes to the file

Shell idea:
    ls definitely_not_here 2> child_err.txt
*/
static void demo_exec_redirect_err(void) {
    pid_t pid = fork();

    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int fd = open("child_err.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            die("open child_err");
        }

        if (dup2(fd, STDERR_FILENO) < 0) {
            close(fd);
            die("dup2 child stderr");
        }
        close(fd);

        execlp("ls", "ls", "definitely_not_here_12345", NULL);
        die("execlp ls error case");
    }

    wait(NULL);
    printf("Finished running error case; check child_err.txt\n");
}

/*
============================================================
10. Simple shell-like example: command < in > out
============================================================
When to use:
Use this when a question asks you to combine redirections.

What this example does:
- creates shell_input.txt
- forks a child
- child redirects stdin from shell_input.txt
- child redirects stdout to shell_output.txt
- child execs wc -w

Shell idea:
    wc -w < shell_input.txt > shell_output.txt

This is a great pattern to study.
*/
static void demo_shell_like(void) {
    int fd = open("shell_input.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    pid_t pid;

    if (fd < 0) {
        die("open shell_input for write");
    }
    write_all(fd, "alpha beta gamma delta epsilon\n", 31);
    close(fd);

    pid = fork();
    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        int in_fd = open("shell_input.txt", O_RDONLY);
        int out_fd = open("shell_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (in_fd < 0) {
            die("open shell_input for read");
        }
        if (out_fd < 0) {
            close(in_fd);
            die("open shell_output for write");
        }

        if (dup2(in_fd, STDIN_FILENO) < 0) {
            close(in_fd);
            close(out_fd);
            die("dup2 stdin shell_like");
        }
        if (dup2(out_fd, STDOUT_FILENO) < 0) {
            close(in_fd);
            close(out_fd);
            die("dup2 stdout shell_like");
        }

        close(in_fd);
        close(out_fd);

        execlp("wc", "wc", "-w", NULL);
        die("execlp wc shell_like");
    }

    wait(NULL);
    printf("Finished shell-like redirection. Check shell_output.txt\n");
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s MODE\n\n"
        "Modes:\n"
        "  dup_basic         - basic dup() example\n"
        "  dup_shared_offset - show shared file position with dup()\n"
        "  stdout_file       - redirect stdout to a file with dup2()\n"
        "  stderr_file       - redirect stderr to a file with dup2()\n"
        "  stdin_file        - redirect stdin from a file with dup2()\n"
        "  restore_stdout    - save and restore stdout\n"
        "  exec_redirect_out - child redirects stdout before exec\n"
        "  exec_redirect_in  - child redirects stdin before exec\n"
        "  exec_redirect_err - child redirects stderr before exec\n"
        "  shell_like        - child does command < in > out\n\n"
        "Examples:\n"
        "  %s dup_basic\n"
        "  %s stdout_file\n"
        "  %s stdin_file\n"
        "  %s restore_stdout\n"
        "  %s exec_redirect_out\n"
        "  %s exec_redirect_in\n"
        "  %s exec_redirect_err\n"
        "  %s shell_like\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "dup_basic") == 0) {
        demo_dup_basic();
    } else if (strcmp(argv[1], "dup_shared_offset") == 0) {
        demo_dup_shared_offset();
    } else if (strcmp(argv[1], "stdout_file") == 0) {
        demo_stdout_file();
    } else if (strcmp(argv[1], "stderr_file") == 0) {
        demo_stderr_file();
    } else if (strcmp(argv[1], "stdin_file") == 0) {
        demo_stdin_file();
    } else if (strcmp(argv[1], "restore_stdout") == 0) {
        demo_restore_stdout();
    } else if (strcmp(argv[1], "exec_redirect_out") == 0) {
        demo_exec_redirect_out();
    } else if (strcmp(argv[1], "exec_redirect_in") == 0) {
        demo_exec_redirect_in();
    } else if (strcmp(argv[1], "exec_redirect_err") == 0) {
        demo_exec_redirect_err();
    } else if (strcmp(argv[1], "shell_like") == 0) {
        demo_shell_like();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
