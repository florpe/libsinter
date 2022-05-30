
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>

#include "lib/mount.h"



int do_unmount(char *sock) {
    printf("Should do unmount here\n");
    errno = ENOSYS;
    return -1;
}

int is_help_flag (char *argv[]) {
    if( strcmp(argv[1], "-h") == 0 ) {
        return 0;
    }
    if( strcmp(argv[1], "--help") == 0 ) {
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {

    int res;
    int mountflags = 0;

    if( argc == 1 || ( argc == 2 && is_help_flag(argv) == 0 ) ) {
        printf("Should print usage of %s here.", argv[0]);
        res = 0;
    }
    else if( strcmp(argv[1], "-m") == 0 ) {
        if( argc != 5 ) {
            fprintf(stderr, "Bad arguments for mount operation.\n");
            return EINVAL;
        }
        mountflags = parse_mountflags(argv[3]);
        res = do_mount(argv[2], argv[4], mountflags);
    }
    else if( strcmp(argv[1], "-e") == 0 ) {
        if( argc < 7 || strcmp(argv[5], "--") != 0 ) {
            fprintf(stderr, "Bad arguments for exec operation.\n");
            return EINVAL;
        }
        mountflags = parse_mountflags(argv[3]);
        res = do_exec(argv[2], argv[4], mountflags, argv + 6);
    }
    else if( strcmp(argv[1], "-u") == 0 ) {
        if( argc != 3 ) {
            fprintf(stderr, "Bad arguments for unmount operation.\n");
            return EINVAL;
        }
        res = do_unmount(argv[2]);
    }
    else {
        fprintf(stderr, "Invalid mode of operation: %s\n", argv[1]);
        return EINVAL;
    }
    return ( res ? errno : 0 ) ;
}

