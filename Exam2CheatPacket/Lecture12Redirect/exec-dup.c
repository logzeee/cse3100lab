#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  pid_t pid = fork();
  if (pid == 0) {
    // wc normally writes to stdout, that is file descriptor 1
    //

    int fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC);

    /* close(1); // 1 no longer writes to stdout */
    /* dup(fd); // dup() will make 1 also point to the same file as fd */
    /* close(fd);// close fd , cause I don't want any other useless file descriptors and only fd 1 will now write to out.txt */

		// we can replace the above commented code using 
		//
		dup2(fd,1);
		close(fd);

    char *argv_list[] = {"wc", argv[1], NULL};
    execvp("wc", argv_list);
    printf("Exec failed\n");
    exit(-1);
  }

  return 0;
}
