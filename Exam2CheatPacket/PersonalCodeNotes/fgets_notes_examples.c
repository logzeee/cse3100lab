/*
Beginner Notes: fgets() in C
============================

This file explains how fgets() works in a simple beginner-friendly way,
with examples you can run.

------------------------------------------------------------
1. What is fgets()?
------------------------------------------------------------
fgets() is used to read a LINE of text from:
    - keyboard (stdin)
    - or a file (FILE *)

It is SAFER than scanf("%s") because:
    - it does NOT overflow your buffer
    - it reads spaces

------------------------------------------------------------
2. Function prototype
------------------------------------------------------------
char *fgets(char *str, int size, FILE *stream);

------------------------------------------------------------
3. How to think about parameters
------------------------------------------------------------
str    = where the text will go (your buffer)
size   = max number of characters to read
stream = where to read from (stdin or a file)

IMPORTANT:
- fgets reads at most size - 1 characters
- it ALWAYS adds '\0' at the end
- it keeps the newline '\n' if there is space

------------------------------------------------------------
4. Return value
------------------------------------------------------------
Returns:
    str  on success
    NULL on error or EOF

------------------------------------------------------------
5. Compile
------------------------------------------------------------
    gcc -Wall -Wextra -std=c11 fgets_notes_examples.c -o fgets_notes

Run:
    ./fgets_notes basic
    ./fgets_notes newline
    ./fgets_notes file
    ./fgets_notes loop
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
============================================================
1. Basic fgets from keyboard
============================================================
*/
void demo_basic() {
    char buffer[50];

    printf("Enter a line: ");
    fgets(buffer, sizeof(buffer), stdin);

    printf("You entered: %s", buffer);
}

/*
============================================================
2. Newline behavior
============================================================
fgets keeps the '\n' if there is space
*/
void demo_newline() {
    char buffer[50];

    printf("Enter a line: ");
    fgets(buffer, sizeof(buffer), stdin);

    printf("Raw string: %s", buffer);
    printf("Length: %lu\n", strlen(buffer));

    // remove newline manually
    buffer[strcspn(buffer, "\n")] = '\0';

    printf("After removing newline: %s\n", buffer);
}

/*
============================================================
3. fgets from file
============================================================
*/
void demo_file() {
    FILE *fp;
    char buffer[100];

    fp = fopen("fgets_demo.txt", "w");
    fprintf(fp, "Line 1\nLine 2\nLine 3\n");
    fclose(fp);

    fp = fopen("fgets_demo.txt", "r");

    fgets(buffer, sizeof(buffer), fp);
    printf("First line: %s", buffer);

    fclose(fp);
}

/*
============================================================
4. Loop reading file line by line
============================================================
Standard pattern
*/
void demo_loop() {
    FILE *fp;
    char buffer[100];

    fp = fopen("fgets_loop.txt", "w");
    fprintf(fp, "A\nB\nC\n");
    fclose(fp);

    fp = fopen("fgets_loop.txt", "r");

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("Read line: %s", buffer);
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s basic|newline|file|loop\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "basic") == 0) {
        demo_basic();
    } else if (strcmp(argv[1], "newline") == 0) {
        demo_newline();
    } else if (strcmp(argv[1], "file") == 0) {
        demo_file();
    } else if (strcmp(argv[1], "loop") == 0) {
        demo_loop();
    } else {
        printf("Invalid option\n");
        return 1;
    }

    return 0;
}
