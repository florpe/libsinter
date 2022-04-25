
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "mount.h"

#define FUSEDEVICE "/dev/fuse"
#define FUSEFSTYPE "fuse"
#define FUSESOURCE "sinter"
#define FUSEFDVAR "FUSEFD"

int check_capabilities() {
    cap_flag_value_t mountcap = 0;
    cap_t proccaps = cap_get_proc();
    if( getuid() == 0 ) {
        return 0;
    }
    if( cap_get_flag(proccaps, CAP_SYS_ADMIN, CAP_PERMITTED, &mountcap) == -1 ) {
        return -1;
    };
    if( mountcap == 0 ) {
        errno = EPERM;
        return -1;
    }
    if( cap_get_flag(proccaps, CAP_SYS_ADMIN, CAP_INHERITABLE, &mountcap) == -1 ) {
        return -1;
    };
    if( mountcap != 0 ) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

int check_mountpoint(char *mnt) {
    //TODO: Factor out messages into an errno->msg function
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

char *check_fusefd_presence(char* options) {

    const char *fd_at_start = "fd=\0";
    const char *fd_with_comma = ",fd=\0";

    char *fdstart;

    if( strncmp(options, fd_at_start, 3) == 0 ) {
        return options+3;
    }
    fdstart = strstr(options, fd_with_comma);
    if( fdstart == NULL ) {
        return NULL;
    }
    return fdstart + 4;

}

int check_fusefd(char* options) {
    int fd;
    char *fdstart;

    fdstart = check_fusefd_presence(options);
    if( fdstart == NULL ) {
        errno = EINVAL;
        printf("Could not find file descriptor in %s\n", options);
        return -1;
    }
    errno = 0;
    fd = strtol(fdstart, NULL, 10);
    if( errno != 0 ) {
        printf("Could not convert file descriptor, errno %i\n", errno);
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

    const char *fusefstype = FUSEFSTYPE;
    const char *fusesource = FUSESOURCE;

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

int do_exec(char *mnt, char* options, int flags, char *execv[]) {
    
    const char *fusefstype = FUSEFSTYPE;
    const char *fusesource = FUSESOURCE;
    const char *fusefdvar = FUSEFDVAR;
    const char *fusedevice = FUSEDEVICE;

    int fusefd;
//    char *fusefdstr;
    int fusefdlen; //TODO: Replace with snprintf(NULL, 0, "%i", fusefd)
    const int inoptlen = strlen(options);
    int mntoptlen;
    char *mntopts;
    int res;
    
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }
    errno = 0;
    if ( check_fusefd_presence(options) != NULL ) {
        printf("File descriptor found in exec mode options.\n");
        errno = EINVAL;
        return -1;
    }
    fusefd = open(fusedevice, O_RDWR);
    fusefdlen = snprintf(NULL, 0, "%i", fusefd);

    if( inoptlen == 0 ) {
        mntoptlen = 3 + fusefdlen + 1;
        mntopts = calloc(mntoptlen, sizeof(char));
        res = sprintf(mntopts, "fd=%i", fusefd);
    }
    else {
        mntoptlen = 3 + fusefdlen + 1 + inoptlen + 1;
        mntopts = calloc(mntoptlen, sizeof(char));
        res = sprintf(mntopts, "fd=%i,%s", fusefd, options);
    }
    if( res == -1 ) {
        printf("Failed to compose, errno %i", errno);
        return -1;
    }
    if( mount(fusesource, mnt, fusefstype, flags, mntopts) == -1 ) {
        printf("Failed to mount: Error %i", errno);
        return -1;
    }
    printf("Mounted!");
    free(mntopts);

    if( check_capabilities() == -1 ) {
        printf("Failed to drop cababilities: Error %i", errno);
        return -1;
    }
//    setenv(fusefdvar, &"FAKEFD", 1); //Can reuse mntopts memory to prepare fd string
    execvp(execv[0], execv);
    return -1;
}
