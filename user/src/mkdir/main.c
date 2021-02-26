#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int
main(int argc, char* argv[])
{
    int fd;
    if (argc <= 1) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0) < 0) {
            fprintf(stderr, "mkdir: cannot mkdir %s\n", argv[i]);
            exit(0);
        } 
    }
    exit(0);
}