/*
Beginner Notes: Unix File APIs in C
===================================

This file is meant to be READ like notes and also used as real code.
It focuses on the common low-level Unix file APIs that classes usually mean by
"Unix file APIs":

    open()
    close()
    read()
    write()
    lseek()
    dup()
    dup2()

It also shows how these fit with:
    file descriptors
    stdin / stdout / stderr
    redirection ideas
    append vs overwrite
    copying files

This file is written for a BEGINNER.
The big comments are the notes.
The functions are the examples.

------------------------------------------------------------
1. Big idea: file descriptor vs FILE *
------------------------------------------------------------
You may already know C standard library functions like:
    fopen, fclose, fprintf, fscanf, fgets

Those use FILE * streams.

Unix file APIs are lower-level.
They use FILE DESCRIPTORS, which are just integers.

Examples:
    0 = stdin
    1 = stdout
    2 = stderr

When you call open(), the OS gives back a new file descriptor like 3, 4, 5, ...

------------------------------------------------------------
2. Standard way to think about each function
------------------------------------------------------------
A. open(path, flags)
B. open(path, flags, mode)

Prototype:
    int open(const char *path, int flags);
    int open(const char *path, int flags, mode_t mode);

How to think about parameters:
    path  = which file you want
    flags = how you want to open it
    mode  = permissions, only needed when creating a new file with O_CREAT

Returns:
    >= 0  file descriptor on success
    -1    on error

Common flags:
    O_RDONLY   read only
    O_WRONLY   write only
    O_RDWR     read and write
    O_CREAT    create if missing
    O_TRUNC    erase old contents when opening for writing
    O_APPEND   always write at the end

B. close(fd)
Prototype:
    int close(int fd);

How to think about parameter:
    fd = the file descriptor to close

Returns:
    0 on success
   -1 on error

C. read(fd, buf, count)
Prototype:
    ssize_t read(int fd, void *buf, size_t count);

How to think about parameters:
    fd    = where to read from
    buf   = memory where bytes should go
    count = max number of bytes to read

Returns:
    >0 number of bytes actually read
     0 EOF (end of file)
    -1 error

D. write(fd, buf, count)
Prototype:
    ssize_t write(int fd, const void *buf, size_t count);

How to think about parameters:
    fd    = where to write to
    buf   = bytes to send
    count = how many bytes to write

Returns:
    >=0 number of bytes actually written
    -1  error

E. lseek(fd, offset, whence)
Prototype:
    off_t lseek(int fd, off_t offset, int whence);

How to think about parameters:
    fd     = which open file
    offset = how far to move
    whence = where to measure from

Common whence values:
    SEEK_SET = from beginning
    SEEK_CUR = from current position
    SEEK_END = from end

Returns:
    new file position on success
    -1 on error

F. dup(oldfd)
Prototype:
    int dup(int oldfd);

How to think about parameter:
    oldfd = file descriptor you want to copy

Returns:
    new file descriptor referring to same open file

G. dup2(oldfd, newfd)
Prototype:
    int dup2(int oldfd, int newfd);

How to think about parameters:
    oldfd = existing fd
    newfd = target number you want to overwrite/make refer to same file

Very important use:
    redirect stdout or stdin

------------------------------------------------------------
3. Compile and run
------------------------------------------------------------
Compile:
    gcc -Wall -Wextra -std=c11 unix_file_apis_notes_examples.c -o unix_files

Run examples:
    ./unix_files open_read
    ./unix_files create_write
    ./unix_files overwrite
    ./unix_files append
    ./unix_files copy
    ./unix_files read_loop
    ./unix_files lseek
    ./unix_files dup
    ./unix_files dup2_stdout
    ./unix_files stdin_read

------------------------------------------------------------
4. Important beginner warnings
------------------------------------------------------------
- Always check return values
- Always close files you open
- read() and write() work with bytes, not formatted text by themselves
- write() may write fewer bytes than requested, so looping is the robust pattern
- read() may read fewer bytes than requested, so loops are common
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
Helper: write all bytes.
Why this matters:
The standard safe pattern is to keep writing until everything is written.
For small examples, one write often works, but this helper is a better model.
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
Helper: copy bytes from one fd to another until EOF.
This is the standard read loop pattern for many file problems.
*/
static void copy_fd_to_fd(int from_fd, int to_fd) {
    char buf[1024];
    ssize_t nr;

    while ((nr = read(from_fd, buf, sizeof(buf))) > 0) {
        write_all(to_fd, buf, (size_t)nr);
    }

    if (nr < 0) {
        die("read");
    }
}

/*
============================================================
1. open() + read() + close()
============================================================
When to use:
Use this when you want to open an existing file and read its contents.
This is the most basic file-reading pattern.

What this example does:
- opens this source file itself for reading
- reads some bytes
- prints them to stdout
- closes the file

Standard pattern:
    fd = open(path, O_RDONLY)
    read(...)
    close(fd)
*/
static void demo_open_read(void) {
    int fd = open("unix_file_apis_notes_examples.c", O_RDONLY);
    char buf[200];
    ssize_t nr;

    if (fd < 0) {
        die("open for read");
    }

    nr = read(fd, buf, sizeof(buf) - 1);
    if (nr < 0) {
        close(fd);
        die("read");
    }

    buf[nr] = '\0';
    printf("Read %zd bytes from file:\n\n", nr);
    printf("%s\n", buf);

    if (close(fd) < 0) {
        die("close");
    }
}

/*
============================================================
2. create file + write to it
============================================================
When to use:
Use this when you need to create a file and put new contents into it.

What this example does:
- creates example_output.txt if needed
- opens it for writing
- truncates old contents if file already existed
- writes text into it

Standard pattern:
    open(path, O_WRONLY | O_CREAT | O_TRUNC, mode)

Important note on mode:
0644 means:
- owner can read/write
- group can read
- others can read
*/
static void demo_create_write(void) {
    int fd = open("example_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    const char *msg = "Hello from create_write\n";

    if (fd < 0) {
        die("open for create_write");
    }

    write_all(fd, msg, strlen(msg));
    printf("Wrote to example_output.txt\n");

    if (close(fd) < 0) {
        die("close");
    }
}

/*
============================================================
3. overwrite example with O_TRUNC
============================================================
When to use:
Use this when you want to replace the old contents completely.

What this example does:
- creates a file with some original text
- opens same file again with O_TRUNC
- writes new text, erasing old contents

Key idea:
O_TRUNC means old contents are cleared when opened for writing.
*/
static void demo_overwrite(void) {
    int fd = open("overwrite_demo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        die("open overwrite first");
    }
    write_all(fd, "OLD CONTENT\n", 12);
    close(fd);

    fd = open("overwrite_demo.txt", O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        die("open overwrite second");
    }
    write_all(fd, "NEW CONTENT\n", 12);
    close(fd);

    printf("overwrite_demo.txt now contains only the new content\n");
}

/*
============================================================
4. append example with O_APPEND
============================================================
When to use:
Use this when you want every write to go at the end of the file.
This is common in logs.

What this example does:
- creates/appends to append_demo.txt
- adds a new line at the end

Key idea:
O_APPEND means writes go to the end instead of overwriting earlier bytes.
*/
static void demo_append(void) {
    int fd = open("append_demo.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    const char *line = "Another appended line\n";

    if (fd < 0) {
        die("open append");
    }

    write_all(fd, line, strlen(line));
    close(fd);
    printf("Appended one line to append_demo.txt\n");
}

/*
============================================================
5. Copy one file to another using read loop + write loop
============================================================
When to use:
Use this for classic file-copy questions.
This is one of the most important patterns.

What this example does:
- opens this source file for reading
- opens copied_output.txt for writing
- copies all bytes until EOF

Standard pattern:
    open source
    open destination
    while ((nr = read(...)) > 0) { write_all(...) }
    close both
*/
static void demo_copy(void) {
    int src = open("unix_file_apis_notes_examples.c", O_RDONLY);
    int dst = open("copied_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (src < 0) {
        die("open source for copy");
    }
    if (dst < 0) {
        close(src);
        die("open destination for copy");
    }

    copy_fd_to_fd(src, dst);

    close(src);
    close(dst);
    printf("Copied file into copied_output.txt\n");
}

/*
============================================================
6. Standard read loop until EOF
============================================================
When to use:
Use this when reading a whole file of unknown size.

What this example does:
- opens this source file
- reads in chunks
- writes chunks to stdout
- stops when read() returns 0 for EOF

Key pattern:
    while ((nr = read(fd, buf, sizeof(buf))) > 0) { ... }
*/
static void demo_read_loop(void) {
    int fd = open("unix_file_apis_notes_examples.c", O_RDONLY);
    char buf[128];
    ssize_t nr;

    if (fd < 0) {
        die("open read_loop");
    }

    while ((nr = read(fd, buf, sizeof(buf))) > 0) {
        write_all(STDOUT_FILENO, buf, (size_t)nr);
    }

    if (nr < 0) {
        close(fd);
        die("read loop");
    }

    close(fd);
}

/*
============================================================
7. lseek()
============================================================
When to use:
Use this when you want to move the file position.
This is common when questions ask you to:
- skip bytes
- reread part of file
- write somewhere specific

What this example does:
- creates a small file
- moves to a certain position
- reads from there

Key idea:
lseek changes where the next read or write happens.
*/
static void demo_lseek(void) {
    int fd = open("lseek_demo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[10];
    ssize_t nr;

    if (fd < 0) {
        die("open lseek write");
    }
    write_all(fd, "ABCDEFGHIJ", 10);
    close(fd);

    fd = open("lseek_demo.txt", O_RDONLY);
    if (fd < 0) {
        die("open lseek read");
    }

    if (lseek(fd, 3, SEEK_SET) == (off_t)-1) {
        close(fd);
        die("lseek");
    }

    nr = read(fd, buf, 4);
    if (nr < 0) {
        close(fd);
        die("read after lseek");
    }

    buf[nr] = '\0';
    printf("After lseek to position 3, next 4 bytes are: %s\n", buf);
    close(fd);
}

/*
============================================================
8. dup()
============================================================
When to use:
Use dup() when you want another file descriptor referring to the same open file.

What this example does:
- opens a file for writing
- duplicates its fd
- writes through both descriptors

Important beginner note:
Both descriptors refer to the same open file description,
so they share the file position.
*/
static void demo_dup(void) {
    int fd1 = open("dup_demo.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int fd2;

    if (fd1 < 0) {
        die("open dup_demo");
    }

    fd2 = dup(fd1);
    if (fd2 < 0) {
        close(fd1);
        die("dup");
    }

    write_all(fd1, "Written via fd1\n", 16);
    write_all(fd2, "Written via fd2\n", 16);

    close(fd1);
    close(fd2);
    printf("dup_demo.txt written using two duplicated descriptors\n");
}

/*
============================================================
9. dup2() to redirect stdout to a file
============================================================
When to use:
Use this when you want normal output functions like printf()
to go to a file instead of the terminal.

What this example does:
- opens a file
- copies that fd onto stdout (fd 1)
- prints using printf
- output goes into the file

Key idea:
After dup2(fd, 1), stdout points to the file.
*/
static void demo_dup2_stdout(void) {
    int fd = open("redirected_stdout.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd < 0) {
        die("open redirected_stdout");
    }

    if (dup2(fd, STDOUT_FILENO) < 0) {
        close(fd);
        die("dup2 stdout");
    }

    close(fd);

    printf("This line goes into redirected_stdout.txt\n");
    fflush(stdout);
}

/*
============================================================
10. Reading from stdin using read()
============================================================
When to use:
Use this when a question asks for raw input from standard input.
This may be keyboard input or redirected input from a file.

What this example does:
- reads bytes from stdin (fd 0)
- prints what was read

Try these:
    ./unix_files stdin_read
    then type something and press Enter

Or:
    echo hello | ./unix_files stdin_read

Key idea:
stdin is just fd 0.
*/
static void demo_stdin_read(void) {
    char buf[100];
    ssize_t nr;

    printf("Type something, then press Enter:\n");
    fflush(stdout);

    nr = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (nr < 0) {
        die("read stdin");
    }

    buf[nr] = '\0';
    printf("You entered: %s\n", buf);
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s MODE\n\n"
        "Modes:\n"
        "  open_read    - open an existing file and read bytes\n"
        "  create_write - create a file and write to it\n"
        "  overwrite    - show O_TRUNC overwriting old contents\n"
        "  append       - show O_APPEND adding to end of file\n"
        "  copy         - copy one file to another\n"
        "  read_loop    - standard read loop until EOF\n"
        "  lseek        - move file position and read from there\n"
        "  dup          - duplicate a file descriptor\n"
        "  dup2_stdout  - redirect stdout to a file\n"
        "  stdin_read   - read from stdin using read()\n\n"
        "Examples:\n"
        "  %s open_read\n"
        "  %s create_write\n"
        "  %s overwrite\n"
        "  %s append\n"
        "  %s copy\n"
        "  %s read_loop\n"
        "  %s lseek\n"
        "  %s dup\n"
        "  %s dup2_stdout\n"
        "  %s stdin_read\n",
        prog, prog, prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "open_read") == 0) {
        demo_open_read();
    } else if (strcmp(argv[1], "create_write") == 0) {
        demo_create_write();
    } else if (strcmp(argv[1], "overwrite") == 0) {
        demo_overwrite();
    } else if (strcmp(argv[1], "append") == 0) {
        demo_append();
    } else if (strcmp(argv[1], "copy") == 0) {
        demo_copy();
    } else if (strcmp(argv[1], "read_loop") == 0) {
        demo_read_loop();
    } else if (strcmp(argv[1], "lseek") == 0) {
        demo_lseek();
    } else if (strcmp(argv[1], "dup") == 0) {
        demo_dup();
    } else if (strcmp(argv[1], "dup2_stdout") == 0) {
        demo_dup2_stdout();
    } else if (strcmp(argv[1], "stdin_read") == 0) {
        demo_stdin_read();
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
