#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* print out an error message and exit */
void my_error(char *s)
{
    perror(s);
    exit(1);
}

/* Concatnate two strings.
 * Dynamically allocate space for the result.
 * Return the address of the result.
 */
char *my_strcat(char *s1, char *s2)
{
    char *result;
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int total_len = len1 + len2 + 1;  // +1 for null terminator
    
    result = (char *) malloc(total_len);
    if (result == NULL) {
        my_error("malloc failed");
    }
    
    strcpy(result, s1);
    strcat(result, s2);
    
    return result;
}

int main(int argc, char *argv[])
{
    char    *s;

    s = my_strcat("", argv[0]);

    for (int i = 1; i < argc; i ++) {
        char *temp = my_strcat(s, argv[i]);
        free(s);
        s = temp;
    }

    printf("%s\n", s);
    free(s);

    return 0;
}
