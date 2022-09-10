
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

#include "lib/mount.h"
#include "lib/sinter.h"

#define FUSEFDVAR "FUSEFD"



int is_flag(char *mode, char *shortflag, char *longflag) {
    if( strcmp(mode, longflag) == 0 ) {
        return 0;
    }
    if( strcmp(mode, shortflag) == 0 ) {
        return 0;
    }
    return -1;
}

int main(int argc, char *argv[]) {
    int uid = getuid();

    int res;
    int flags = 0;
    char *mnt_abs;
    char *mode = argv[1];

    if( check_capabilities() == -1 ) {
        fprintf(stderr, "Bad cababilities: Error %i\n", errno);
        return -1;
    }
    if( mode == NULL || ( argc == 2 && is_flag(mode, "-h", "--help") == 0 ) ) {
        printf("Usage:\n");
        printf("    %s [ -m | --mount ] MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE\n", argv[0]);
        printf("        Create mount at MOUNTPOINT using MOUNTCOOKIE for identification. The fd=X option in FUSEOPTIONS is required.\n");
        printf("    %s [ -e | --exec ] MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE -- EXEC [ ARG ... ]\n", argv[0]);
        printf("        Create mount at MOUNTPOINT, store the file descriptor in $FUSEFD, then execute CMD [ ARG ... ] . The fd=X option in FUSEOPTIONS must not be set.\n");
        printf("    %s [ -f | --files-exec ] FILE [ FILE ... ] -- EXEC [ ARG ... ]\n", argv[0]);
        printf("        Read mount configurations line by line from FILEs until it encounters the -- argument, then execute CMD [ ARG ... ] .\n");
        printf("        Each mount configuration line consists of a file descriptor or environment variable name followed by arguments as for the --mount option.\n");
        printf("    %s [ -g | --files-noexec ] FILE [ FILE ... ]\n", argv[0]);
        printf("        Like --files-noexec, but raises an error if it encounters the -- argument.\n");
        printf("    %s [ -u | --unmount ] MOUNTPOINT UMOUNTFLAGS MOUNTCOOKIE\n", argv[0]);
        printf("        Unmount the topmpost FUSE filesystem at MOUNTPOINT identified by MOUNTCOOKIE.\n");
        printf("    %s [ -h | --help ]\n", argv[0]);
        printf("        Show this help.\n");
        res = 0;
    }
    else if( is_flag(mode, "-m", "--mount") == 0 ) {
        if( argc != 6 ) {
            fprintf(stderr, "Bad arguments for mount operation.\n");
            return EINVAL;
        }
        flags = parse_mountflags(argv[3]);
        res = do_mount(argv[2], argv[4], flags, argv[5]);
    }
    else if( is_flag(mode, "-e", "--execute") == 0 ) {
        if( argc < 8 || strcmp(argv[6], "--") != 0 ) {
            fprintf(stderr, "Bad arguments for exec operation.\n");
            return EINVAL;
        }
        flags = parse_mountflags(argv[3]);
        res = do_exec(argv[2], argv[4], flags, FUSEFDVAR, argv[5], argv + 7);
    }
    else if( is_flag(mode, "-f", "--files-exec") == 0 ) {
        if( argc < 3 ) {
            fprintf(stderr, "Bad arguments for file-based operation.\n");
            return EINVAL;
        }
        res = do_fromfiles_exec(argc - 2, argv + 2);
    }
    else if( is_flag(mode, "-g", "--files-noexec") == 0 ) {
        if( argc < 3 ) {
            fprintf(stderr, "Bad arguments for file-based operation.\n");
            return EINVAL;
        }
        res = do_fromfiles_noexec(argc - 2, argv + 2);
    }
    else if( strcmp(mode, "-u") == 0 ) {
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
        fprintf(stderr, "Invalid mode of operation: %s\n", mode);
        return EINVAL;
    }
    return ( res ? errno : 0 ) ;
}

