
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

#include "lib/mount.h"




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
    int uid = getuid();

    int res;
    int flags = 0;
    char *mnt_abs;

    if( argc == 1 || ( argc == 2 && is_help_flag(argv) == 0 ) ) {
        printf("Should print usage of %s here.", argv[0]);
        res = 0;
    }
    else if( strcmp(argv[1], "-m") == 0 ) {
        if( argc != 6 ) {
            fprintf(stderr, "Bad arguments for mount operation.\n");
            return EINVAL;
        }
        flags = parse_mountflags(argv[3]);
        res = do_mount(argv[2], argv[4], flags, argv[5]);
    }
    else if( strcmp(argv[1], "-e") == 0 ) {
        if( argc < 8 || strcmp(argv[6], "--") != 0 ) {
            fprintf(stderr, "Bad arguments for exec operation.\n");
            return EINVAL;
        }
        flags = parse_mountflags(argv[3]);
        res = do_exec(argv[2], argv[4], flags, argv[5], argv + 7);
    }
    else if( strcmp(argv[1], "-u") == 0 ) {
        if( argc != 4 ) {
            fprintf(stderr, "Bad arguments for umount operation.\n");
            return EINVAL;
        }
        mnt_abs = realpath(argv[2], NULL);
        if( mnt_abs == NULL ) {
            printf("Bad umount path\n");
            return EINVAL;
        }
        flags = parse_umountflags(argv[3]);
        if( flags == -1 ) {
            printf("Bad umount flags\n");
            return EINVAL;
        }
        res = do_umount(uid, mnt_abs, flags);
    } else {
        fprintf(stderr, "Invalid mode of operation: %s\n", argv[1]);
        return EINVAL;
    }
    return ( res ? errno : 0 ) ;
}

