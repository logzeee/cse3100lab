/*
Beginner Notes: The exec Family in C
====================================

This file is meant to be READ like notes and also used as real code.
It explains the main exec-family functions you are likely to see in class:

    execl
    execlp
    execv
    execvp
    execle
    execve

Important note:
There is NO standard function called "execlv".
People sometimes mix up the names, but the common ones are:
    l  = list of arguments written one by one
    v  = vector (array) of arguments
    p  = search PATH for the program name
    e  = provide a custom environment

Big idea of exec
----------------
exec does NOT create a new process.
It REPLACES the current process with a new program.

That means this pattern is common:
    1. fork() creates a child process
    2. child calls exec...
    3. child becomes a different program

If exec succeeds:
    - it does not return
    - your old code is gone from that process
    - control jumps to the new program's main()

If exec fails:
    - it returns -1
    - errno is set

Mental model for the letters
----------------------------
    l = list
        You type arguments directly in the function call.

    v = vector
        You put arguments in a char* array and pass the array.

    p = PATH
        Search for the program in the PATH environment variable.
        Good for commands like "ls", "wc", "grep".

    e = environment
        You pass a custom environment array.

Very important argv rule
------------------------
For exec, the first argument INSIDE the new program is usually argv[0],
which is usually the program name.

Example:
    execlp("ls", "ls", "-l", NULL);

Inside the new program, argv usually looks like:
    argv[0] = "ls"
    argv[1] = "-l"
    argv[2] = NULL

Build and run idea
------------------
This file is mainly notes + examples.
You can compile it:
    gcc -Wall -Wextra -std=c11 exec_family_notes_examples.c -o exec_notes

Then run different demos like:
    ./exec_notes execl
    ./exec_notes execlp
    ./exec_notes execv
    ./exec_notes execvp
    ./exec_notes execle
    ./exec_notes execve
    ./exec_notes fork_exec

Some demos use programs like /bin/echo, /bin/ls, or printenv.
Those are common on Linux systems.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/*
Helper: print error and exit.
We use perror so you can see why exec failed.
*/
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
Helper: wait for child and report status.
Useful in the fork+exec demo.
*/
static void wait_for_child(pid_t pid) {
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        die("waitpid");
    }

    if (WIFEXITED(status)) {
        printf("\n[parent] child exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("\n[parent] child killed by signal %d\n", WTERMSIG(status));
    } else {
        printf("\n[parent] child ended in some other way\n");
    }
}

/*
============================================================
1. execl
============================================================
Prototype:
    int execl(const char *path, const char *arg0, ..., (char *)NULL);

How to think about parameters:
    path  = exact path to the program
    arg0  = what the new program sees as argv[0]
    ...   = more arguments, written one by one
    NULL  = marks the end of the argument list

Use execl when:
    - you know the exact path
    - you know all arguments at coding time

Example shell command idea:
    /bin/echo hello world
*/
static void demo_execl(void) {
    printf("About to call execl...\n");
    printf("If execl succeeds, this function stops here and becomes /bin/echo.\n\n");

    execl("/bin/echo",   /* exact path to program */
          "echo",        /* argv[0] inside new program */
          "Hello",       /* argv[1] */
          "from",        /* argv[2] */
          "execl",       /* argv[3] */
          (char *)NULL);  /* end of argument list */

    /* Only runs if exec failed */
    die("execl");
}

/*
============================================================
2. execlp
============================================================
Prototype:
    int execlp(const char *file, const char *arg0, ..., (char *)NULL);

How to think about parameters:
    file  = program name to search in PATH
    arg0  = argv[0]
    ...   = more arguments written one by one
    NULL  = end

Difference from execl:
    execl  needs an exact path like /bin/ls
    execlp can search PATH for something like "ls"

Use execlp when:
    - you want shell-like command lookup
    - you know arguments at coding time

Example shell command idea:
    ls -l
*/
static void demo_execlp(void) {
    printf("About to call execlp...\n");
    printf("execlp will search PATH for 'ls'.\n\n");

    execlp("ls",         /* program name, searched in PATH */
           "ls",         /* argv[0] */
           "-l",         /* argv[1] */
           (char *)NULL); /* end */

    die("execlp");
}

/*
============================================================
3. execv
============================================================
Prototype:
    int execv(const char *path, char *const argv[]);

How to think about parameters:
    path  = exact path to the program
    argv  = array of strings
            argv[0] should usually be the program name
            argv[last] must be NULL

Use execv when:
    - you know the exact path
    - arguments are easier to build in an array
    - maybe number of args is not convenient to type manually

Example shell command idea:
    /bin/echo hello from execv
*/
static void demo_execv(void) {
    char *args[] = {
        "echo",   /* argv[0] */
        "Hello",  /* argv[1] */
        "from",   /* argv[2] */
        "execv",  /* argv[3] */
        NULL       /* end of array */
    };

    printf("About to call execv...\n");
    printf("Arguments are passed as an array.\n\n");

    execv("/bin/echo", args);

    die("execv");
}

/*
============================================================
4. execvp
============================================================
Prototype:
    int execvp(const char *file, char *const argv[]);

How to think about parameters:
    file  = program name to search in PATH
    argv  = array of strings ending with NULL

This is one of the most common and useful versions.

Use execvp when:
    - you want PATH search
    - arguments are in an array
    - you are building a shell or shell-like launcher

Example shell command idea:
    wc -w /etc/passwd

This may vary by system, but /etc/passwd is commonly present on Linux/Unix.
*/
static void demo_execvp(void) {
    char *args[] = {
        "wc",          /* argv[0] */
        "-w",          /* argv[1] */
        "/etc/passwd", /* argv[2] */
        NULL
    };

    printf("About to call execvp...\n");
    printf("execvp searches PATH for 'wc'.\n\n");

    execvp("wc", args);

    die("execvp");
}

/*
============================================================
5. execle
============================================================
Prototype:
    int execle(const char *path, const char *arg0, ...,
               (char *)NULL, char *const envp[]);

How to think about parameters:
    path  = exact path to the program
    arg0  = argv[0]
    ...   = normal arguments one by one
    NULL  = end of argument list
    envp  = custom environment array

This one is like execl, BUT you also pass a custom environment.

Important:
    There are TWO endings here conceptually:
    1. NULL ends the argument list
    2. envp is passed after that NULL

Use execle when:
    - you know exact path
    - want list-style args
    - want custom environment

Example idea:
    run /usr/bin/env or /usr/bin/printenv with custom variable

We'll use /usr/bin/env because it commonly prints the environment.
*/
static void demo_execle(void) {
    char *custom_env[] = {
        "MY_NAME=Thembi",
        "COURSE=C Programming",
        "LEVEL=Beginner",
        NULL
    };

    printf("About to call execle...\n");
    printf("This runs a program with a custom environment.\n\n");

    execle("/usr/bin/env", /* exact path */
           "env",          /* argv[0] */
           (char *)NULL,    /* end of args */
           custom_env);     /* custom environment */

    die("execle");
}

/*
============================================================
6. execve
============================================================
Prototype:
    int execve(const char *path, char *const argv[], char *const envp[]);

How to think about parameters:
    path  = exact path to program
    argv  = argument array ending with NULL
    envp  = environment array ending with NULL

This is the most direct low-level form.
Many other exec functions are wrappers around behavior like this.

Use execve when:
    - you want exact path
    - args in array
    - custom environment
    - low-level control
*/
static void demo_execve(void) {
    char *args[] = {
        "env",
        NULL
    };

    char *custom_env[] = {
        "ANIMAL=lion",
        "COLOR=blue",
        NULL
    };

    printf("About to call execve...\n");
    printf("This is a lower-level exec with argv + envp arrays.\n\n");

    execve("/usr/bin/env", args, custom_env);

    die("execve");
}

/*
============================================================
7. The most common real pattern: fork + exec
============================================================
This is the pattern you should probably remember first.

Why?
Because exec replaces the current process.
If the parent called exec directly, the parent would disappear into
that new program.

So usually:
    parent forks
    child execs
    parent waits

We will use execvp here because it is extremely common.
*/
static void demo_fork_exec(void) {
    pid_t pid = fork();

    if (pid < 0) {
        die("fork");
    }

    if (pid == 0) {
        /* child */
        char *args[] = {
            "echo",
            "Hello",
            "from",
            "child",
            "using",
            "fork+execvp",
            NULL
        };

        printf("[child] I am about to become the echo program.\n");
        execvp("echo", args);

        /* only if exec fails */
        die("execvp in child");
    }

    /* parent continues here */
    printf("[parent] I created a child with pid %ld\n", (long)pid);
    printf("[parent] I am waiting for the child to finish...\n");
    wait_for_child(pid);
}

/*
============================================================
8. Tiny comparison examples side-by-side
============================================================
These are not run directly here, but they are good notes.

A. execl
    execl("/bin/echo", "echo", "hi", "there", NULL);

B. execlp
    execlp("echo", "echo", "hi", "there", NULL);

C. execv
    char *args[] = {"echo", "hi", "there", NULL};
    execv("/bin/echo", args);

D. execvp
    char *args[] = {"echo", "hi", "there", NULL};
    execvp("echo", args);

E. execle
    char *env[] = {"X=123", NULL};
    execle("/usr/bin/env", "env", NULL, env);

F. execve
    char *args[] = {"env", NULL};
    char *env[]  = {"X=123", NULL};
    execve("/usr/bin/env", args, env);

Quick way to remember
---------------------
    execl   = exact path + list
    execlp  = PATH search + list
    execv   = exact path + vector(array)
    execvp  = PATH search + vector(array)
    execle  = exact path + list + environment
    execve  = exact path + vector(array) + environment

Notice:
    There is no standard execlpv or execlv in the usual family.
*/

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s MODE\n\n"
        "Modes:\n"
        "  execl      - exact path + list arguments\n"
        "  execlp     - PATH search + list arguments\n"
        "  execv      - exact path + argv array\n"
        "  execvp     - PATH search + argv array\n"
        "  execle     - exact path + list args + custom environment\n"
        "  execve     - exact path + argv array + custom environment\n"
        "  fork_exec  - classic fork + exec example\n\n"
        "Examples:\n"
        "  %s execl\n"
        "  %s execlp\n"
        "  %s execv\n"
        "  %s execvp\n"
        "  %s execle\n"
        "  %s execve\n"
        "  %s fork_exec\n",
        prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "execl") == 0) {
        demo_execl();
    } else if (strcmp(argv[1], "execlp") == 0) {
        demo_execlp();
    } else if (strcmp(argv[1], "execv") == 0) {
        demo_execv();
    } else if (strcmp(argv[1], "execvp") == 0) {
        demo_execvp();
    } else if (strcmp(argv[1], "execle") == 0) {
        demo_execle();
    } else if (strcmp(argv[1], "execve") == 0) {
        demo_execve();
    } else if (strcmp(argv[1], "fork_exec") == 0) {
        demo_fork_exec();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    /*
    In normal exec demos, code does not get here if exec succeeds,
    because the process has been replaced.
    */
    return EXIT_SUCCESS;
}
