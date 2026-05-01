#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define RD 0
#define WR 1



int main(int argc, char *argv[]){
    if (argc <= 1) 
        die ("write at least one character");
    
    int p_to_c1[2];
    int c1_to_c2[2];
    int c2_to_p[2];
    int p_to_c2[2];
    int c2_to_c1[2];
    int c1_to_p[2];
    
    pid_t pid_a;
    pid_t pid_b;
    pid_t pid_c;

    if (pipe(p_to_c1[2]) == -1) {
        die("pipe fail");
    }
    if (pipe(c1_to_c2[2]) == -1) {
        die("pipe fail");
    }
    if (pipe(c2_to_p[2]) == -1) {
        die("pipe fail");
    }
    if (pipe(p_to_c2[2]) == -1) {
        die("pipe fail");
    }
    if (pipe(c2_to_c1[2]) == -1) {
        die("pipe fail");
    }
    if (pipe(c1_to_p[2]) == -1) {
        die("pipe fail");
    }


    pid_a = fork();
    if (pid_a < 0) {
        die("fork A");
    }
    if (pid_a == 0) {
        // A
        //receives from parent and saves, does not write, only reads
        close(p_to_c1[WR]);
        dup2(p_to_c1[RD], 1);
        close(p_to_c1[RD]);

    }

    //read from command line and send to c1
    close(p_to_c1[RD]);
    char letter = argv[1];
    write(p_to_c1[WR], letter, sizeof(letter));
    close(p_to_c1[WR]);