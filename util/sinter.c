
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>

#include "lib/mount.h"


int do_exec(char *mnt, char* options, int flags, int execc, char *execv[]) {
    printf("Should do exec here\n");
    return -1;
}

int do_socket(char *sock, char* options, int flags, char* socketpath) {
    printf("Should run socket here\n");
    return -1;
}

int do_unmount(char *sock) {
    printf("Should do unmount here\n");
    return -1;
}

int main(int argc, char *argv[]) {

    int res;

    printf("Hello, world!\n");
    if( argc == 1 ) {
        printf("Should print usage of %s here.", argv[0]);
        res = 0;
    }
    if( strcmp(argv[1], "-m") == 0 ) {
        if( argc != 4 ) {
            printf("Bad arguments for mount operation.\n");
            return EINVAL;
        }
        res = do_mount(argv[2], argv[3], MS_RDONLY | MS_NOSUID);
    }
    else if( strcmp(argv[1], "-e") == 0 ) {
        if( argc < 6 || strcmp(argv[4], "--") != 0 ) {
            printf("Bad arguments for exec operation.\n");
            return EINVAL;
        }
        res = do_exec(argv[2], argv[3], MS_RDONLY | MS_NOSUID, argc - 5, argv + 5);
    }
    else if( strcmp(argv[1], "-s") == 0 ) {
        if( argc != 5 ) {
            printf("Bad arguments for socket operation.\n");
            return EINVAL;
        }
        res = do_socket(argv[2], argv[3], MS_RDONLY | MS_NOSUID, argv[4]);
    }
    else if( strcmp(argv[1], "-u") == 0 ) {
        if( argc != 3 ) {
            printf("Bad arguments for unmount operation.\n");
            return EINVAL;
        }
        res = do_unmount(argv[2]);
    }
    else {
        printf("Invalid mode of operation: %s\n", argv[1]);
        errno = EINVAL;
        res = -1;
    }
    return ( res ? errno : 0 ) ;
}

