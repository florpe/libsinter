
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/stat.h>

#include "mount.h"

int check_mountpoint(char *mnt) {

    struct stat mntstat = mntstat;

    if( getuid() == 0 ) {
        return 0;
    }

    if( lstat(mnt, &mntstat) == -1 ) {
        printf("Could not stat %s, error %i\n", mnt, errno);
        return -1;
    }
    if(!S_ISDIR(mntstat.st_mode)) {
        errno = ENOTDIR;
        printf("Mountpoint must be a directory.\n");
        return -1;
    }
    if(mntstat.st_uid != getuid()) {
        errno = EACCES;
        printf("Mountpoint must be owned by invoking user.\n");
        return -1;
    }
    if(mntstat.st_mode && S_IRWXU != S_IRWXU ) {
        errno = EACCES;
        printf("Mountpoint must have u+rwx permissions.\n");
        return -1;
    }
    printf("Mount point looks good.\n");
    return 0;
}

int check_fusefd(char* options) {
    int fd;
    char *fdstart;

    const char *fd_at_start = "fd=\0";
    const char *fd_with_comma = ",fd=\0";

    if( strncmp(options, fd_at_start, 3) == 0 ) {
        fdstart = options+3;
    }
    else {
        fdstart = strstr(options, fd_with_comma);
        if( fdstart == NULL ) {
            errno = EINVAL;
            printf("Could not find file descriptor in %s", options);
            return -1;
        }
        fdstart = fdstart + 4;
    }
    errno = 0;
    fd = strtol(fdstart, NULL, 10);
    if( errno != 0 ) {
        printf("Could not convert file descriptor, errno %i", errno);
        return -1;
    }
    if( fcntl(fd, F_GETFD) ==  -1) {
        errno = EINVAL;
        printf("Bad file descriptor: %i\n", fd);
        return -1;
    }
    //TODO: Check that fd belongs to /dev/fuse ?
    printf("Found file descriptor: %i\n", fd);
    return 0;
}

int do_mount(char *mnt, char* options, int flags) {

    const char *fusefstype = "fuse";
    const char *fusesource = "sinter";

    if(check_fusefd(options) == -1 ) {
        return -1;
    }
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }
    if ( mount(fusesource, mnt, fusefstype, flags, options) == -1 ) {
        printf("Failed to mount: Error %i", errno);
        return -1;
    }
    printf("Mounted!");

    return 0;
}
