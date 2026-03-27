#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

/*
When to use:
Use execl when you know the exact full path to the program
and you want to write the arguments directly one by one in the code.

What this example does:
This replaces the current process with /bin/echo.
The new program acts like running:
    /bin/echo hello from execl

Why this is a good beginner example:
It shows the "l" version, where arguments are written as a list.
*/
void run_execl(void) {
    execl("/bin/echo", "echo", "hello", "from", "execl", (char *)NULL);
    perror("execl failed");
    exit(1);
}

/*
When to use:
Use execlp when you want the system to search for the program in PATH,
like how the shell finds commands such as echo, ls, or wc.
Also use it when you want to write the arguments directly in the code.

What this example does:
This replaces the current process with echo found through PATH.
The new program acts like running:
    echo hello from execlp

Why this is a good beginner example:
It shows the "p" version, which searches PATH,
and the "l" version, which takes arguments as a list.
*/
void run_execlp(void) {
    execlp("echo", "echo", "hello", "from", "execlp", (char *)NULL);
    perror("execlp failed");
    exit(1);
}

/*
When to use:
Use execv when you know the exact full path to the program,
but your arguments are easier to store in an array instead of typing
one by one in the function call.

What this example does:
This replaces the current process with /bin/echo.
The new program acts like running:
    /bin/echo hello from execv

Why this is a good beginner example:
It shows the "v" version, where arguments are stored in a char* array.
That is useful when arguments come from another function or are built first.
*/
void run_execv(void) {
    char *args[] = {"echo", "hello", "from", "execv", NULL};
    execv("/bin/echo", args);
    perror("execv failed");
    exit(1);
}

/*
When to use:
Use execvp when you want both of these:
1. PATH searching for the command name
2. arguments stored in an array

This is one of the most common and practical exec versions.
It is very useful for shell-like programs.

What this example does:
This replaces the current process with echo found through PATH.
The new program acts like running:
    echo hello from execvp

Why this is a good beginner example:
It combines the two most flexible ideas:
- "p" means search PATH
- "v" means use an argv-style array
*/
void run_execvp(void) {
    char *args[] = {"echo", "hello", "from", "execvp", NULL};
    execvp("echo", args);
    perror("execvp failed");
    exit(1);
}

/*
When to use:
Use the fork + exec pattern when the parent process should stay alive,
but the child process should turn into another program.
This is the normal real-world pattern used by shells.

What this example does:
The parent creates a child.
The child becomes the echo program and runs something like:
    echo hello from child using execvp
The parent waits for the child to finish, then prints a message.

Why this is a good beginner example:
It shows the most important practical idea:
- fork creates a new process
- exec replaces the child with a new program
- wait lets the parent wait for the child to finish
*/
void run_fork_exec(void) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        char *args[] = {"echo", "hello", "from", "child", "using", "execvp", NULL};
        execvp("echo", args);
        perror("execvp failed");
        exit(1);
    }

    wait(NULL);
    printf("parent: child finished\n");
}

/*
When to use this struct pattern:
Use a struct like this when you want to package a command neatly,
especially when a function should receive both the program name
and its argument array together.

What this struct represents:
program = the command name to run
args    = the argv-style array to pass into execvp

Why this is useful:
It makes fork/exec code cleaner when commands are built somewhere else
and then passed into a helper function.
*/
typedef struct {
    const char *program;
    char *const *args;
} ExecCommand;

/*
When to use:
Use this pattern when command information is already packaged in a struct.
This is useful if your program is reading commands from somewhere else,
like a parser, config file, or user input handler.

What this example does:
This forks a child.
The child uses execvp with data stored in the struct.
The parent waits for the child to finish.

Why this is a good beginner example:
It shows that exec does not care whether arguments came from hard-coded code
or from a struct, as long as you give it the right program name and argv array.
*/
void run_fork_exec_struct(ExecCommand cmd) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        execvp(cmd.program, cmd.args);
        perror("execvp failed");
        exit(1);
    }

    wait(NULL);
    printf("parent: struct-based child finished
");
}

/*
When to use:
Use this pattern when the exact program path and its arguments
come from the command line instead of being hard-coded in the source code.

What this example does:
If you run something like:
    ./exec_notes execv_input /bin/echo hello from execv_input
then this function runs the equivalent of:
    /bin/echo hello from execv_input

How it works:
argv[2] is treated as the program path.
&argv[2] is used as the new argv array passed to execv.
That means the new program sees:
    argv[0] = "/bin/echo"
    argv[1] = "hello"
    argv[2] = "from"
    argv[3] = "execv_input"

Why this is a good beginner example:
It shows that execv can reuse part of the existing command-line argv array.
*/
void run_execv_from_input(int argc, char *argv[]) {
    execv(argv[2], &argv[2]);
    perror("execv failed");
    exit(1);
}

/*
When to use:
Use this pattern when the command and its arguments come from the command line,
and you want PATH search instead of requiring a full path.

What this example does:
If you run:
    ./exec_notes execvp_input echo hello from execvp_input
then this function runs the equivalent of:
    echo hello from execvp_input

How it works:
argv[2] is the command name.
&argv[2] is the argv array given to execvp.
Because this is execvp, the system searches PATH for the command.

Why this is a good beginner example:
It shows how a simple shell-like launcher can pass user input directly into execvp.
*/
void run_execvp_from_input(int argc, char *argv[]) {
    execvp(argv[2], &argv[2]);
    perror("execvp failed");
    exit(1);
}

/*
When to use:
Use this pattern when a parent process should stay alive,
and the child should execute a command provided by user input.
This is very close to how a tiny shell would work.

What this example does:
If you run:
    ./exec_notes fork_exec_input echo hello from fork_exec_input
then the parent forks.
The child becomes the echo program and runs:
    echo hello from fork_exec_input
The parent waits for the child to finish.

Why this is a good beginner example:
It combines the two most important real-world ideas:
- input comes from command line
- child uses execvp to run that input as a command
*/
void run_fork_exec_from_input(int argc, char *argv[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        execvp(argv[2], &argv[2]);
        perror("execvp failed");
        exit(1);
    }

    wait(NULL);
    printf("parent: command-line child finished
");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage:
");
        printf("  %s execl
", argv[0]);
        printf("  %s execlp
", argv[0]);
        printf("  %s execv
", argv[0]);
        printf("  %s execvp
", argv[0]);
        printf("  %s fork_exec
", argv[0]);
        printf("  %s execv_input /bin/echo hello from execv_input
", argv[0]);
        printf("  %s execvp_input echo hello from execvp_input
", argv[0]);
        printf("  %s fork_exec_input echo hello from fork_exec_input
", argv[0]);
        printf("  %s struct_demo
", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "execl") == 0) {
        run_execl();
    } else if (strcmp(argv[1], "execlp") == 0) {
        run_execlp();
    } else if (strcmp(argv[1], "execv") == 0) {
        run_execv();
    } else if (strcmp(argv[1], "execvp") == 0) {
        run_execvp();
    } else if (strcmp(argv[1], "fork_exec") == 0) {
        run_fork_exec();
    } else if (strcmp(argv[1], "execv_input") == 0) {
        if (argc < 3) {
            printf("Usage: %s execv_input /full/path arg1 arg2 ...
", argv[0]);
            return 1;
        }
        run_execv_from_input(argc, argv);
    } else if (strcmp(argv[1], "execvp_input") == 0) {
        if (argc < 3) {
            printf("Usage: %s execvp_input command arg1 arg2 ...
", argv[0]);
            return 1;
        }
        run_execvp_from_input(argc, argv);
    } else if (strcmp(argv[1], "fork_exec_input") == 0) {
        if (argc < 3) {
            printf("Usage: %s fork_exec_input command arg1 arg2 ...
", argv[0]);
            return 1;
        }
        run_fork_exec_from_input(argc, argv);
    } else if (strcmp(argv[1], "struct_demo") == 0) {
        char *args[] = {"echo", "hello", "from", "struct", "pattern", NULL};
        ExecCommand cmd = {"echo", args};
        run_fork_exec_struct(cmd);
    } else {
        printf("Invalid option
");
        return 1;
    }

    return 0;
}
