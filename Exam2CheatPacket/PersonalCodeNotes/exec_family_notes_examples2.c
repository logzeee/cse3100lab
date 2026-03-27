#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_execl(void) {
    execl("/bin/echo", "echo", "hello", "from", "execl", (char *)NULL);
    perror("execl failed");
    exit(1);
}

void run_execlp(void) {
    execlp("echo", "echo", "hello", "from", "execlp", (char *)NULL);
    perror("execlp failed");
    exit(1);
}

void run_execv(void) {
    char *args[] = {"echo", "hello", "from", "execv", NULL};
    execv("/bin/echo", args);
    perror("execv failed");
    exit(1);
}

void run_execvp(void) {
    char *args[] = {"echo", "hello", "from", "execvp", NULL};
    execvp("echo", args);
    perror("execvp failed");
    exit(1);
}

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

typedef struct {
    const char *program;
    char *const *args;
} ExecCommand;

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

void run_execv_from_input(int argc, char *argv[]) {
    execv(argv[2], &argv[2]);
    perror("execv failed");
    exit(1);
}

void run_execvp_from_input(int argc, char *argv[]) {
    execvp(argv[2], &argv[2]);
    perror("execvp failed");
    exit(1);
}

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
