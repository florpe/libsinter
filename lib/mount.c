
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mntent.h>
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
#define PROCMOUNTS "/proc/mounts"

#define FUSEFDVAR "FUSEFD"

#define FUSEFSTYPE "fuse"
#define FUSESOURCE_FMT_PREFIX "sinter-0x%.4X-"
#define FUSESOURCE_FMT_ENTIRE "sinter-0x%.4X-%s"

char *make_source_entry(int uid, char *suffix) {
    /* Wrapper for creating the mnt_fsname entry in PROCMOUNTS. Returns
     * a pointer to the complete entry string, which must be deallocated
     * after use.
     * If suffix is NULL, returns only the prefix.
     */
    const int len = 1 + (
        ( suffix == NULL)
            ? snprintf(NULL, 0, FUSESOURCE_FMT_PREFIX, uid)
            : snprintf(NULL, 0, FUSESOURCE_FMT_ENTIRE, uid, suffix)
        );
    char *res = calloc(len, sizeof(char));
    if( res == NULL ) {
        return NULL;
    }
    ( suffix == NULL ) ?
        sprintf(res, FUSESOURCE_FMT_PREFIX, uid)
        : sprintf(res, FUSESOURCE_FMT_ENTIRE, uid, suffix)
        ;
    return res;
}

int check_capabilities() {
    /* Make sure that the running process is able to mount using
     * CAP_SYS_ADMIN, but will lose that capability on exec() .
     *
     * TODO: Set up tests around this using capsh. Remember to set
     * a non-zero uid to avoid the short-circuit for root below!
     */
    cap_t proccaps;
    cap_flag_value_t mountcap = 0;
    int res = 0;

    if( getuid() == 0 ) {
        return 0;
    }

    proccaps = cap_get_proc();
    if( cap_get_flag(proccaps, CAP_SYS_ADMIN, CAP_PERMITTED, &mountcap) == -1 ) {
        res = -1;
    }
    else if( mountcap == 0 ) {
        errno = EPERM;
        res = -1;
    }
    else if( cap_get_flag(proccaps, CAP_SYS_ADMIN, CAP_INHERITABLE, &mountcap) == -1 ) {
        res = -1;
    }
    else if( mountcap != 0 ) {
        errno = EINVAL;
        res = -1;
    }
    cap_free(proccaps);
    return res;
}

int parse_mountflags(char *flags) {
    /* TODO: This is an atrocity.
     * NOSUIDIRTIME would parse.
     */
    int res = 0;
    if( strstr(flags, "REMOUNT") != NULL ) {
        res |= MS_REMOUNT;
    }
    if( strstr(flags, "BIND") != NULL ) {
        res |= MS_BIND;
    }
    if( strstr(flags, "SHARED") != NULL ) {
        res |= MS_SHARED;
    }
    if( strstr(flags, "PRIVATE") != NULL ) {
        res |= MS_PRIVATE;
    }
    if( strstr(flags, "SLAVE") != NULL ) {
        res |= MS_SLAVE;
    }
    if( strstr(flags, "UNBINDABLE") != NULL ) {
        res |= MS_UNBINDABLE;
    }
    if( strstr(flags, "MOVE") != NULL ) {
        res |= MS_MOVE;
    }
    if( strstr(flags, "DIRSYNC") != NULL ) {
        res |= MS_DIRSYNC;
    }
    if( strstr(flags, "LAZYTIME") != NULL ) {
        res |= MS_LAZYTIME;
    }
    if( strstr(flags, "MANDLOCK") != NULL ) {
        res |= MS_MANDLOCK;
    }
    if( strstr(flags, "NOATIME") != NULL ) {
        res |= MS_NOATIME;
    }
    if( strstr(flags, "NODEV") != NULL ) {
        res |= MS_NODEV;
    }
    if( strstr(flags, "NODIRATIME") != NULL ) {
        res |= MS_NODIRATIME;
    }
    if( strstr(flags, "NOEXEC") != NULL ) {
        res |= MS_NOEXEC;
    }
    if( strstr(flags, "NOSUID") != NULL ) {
        res |= MS_NOSUID;
    }
    if( strstr(flags, "RDONLY") != NULL ) {
        res |= MS_RDONLY;
    }
    if( strstr(flags, "REC") != NULL ) {
        res |= MS_REC;
    }
    if( strstr(flags, "RELATIME") != NULL ) {
        res |= MS_RELATIME;
    }
    if( strstr(flags, "SILENT") != NULL ) {
        res |= MS_SILENT;
    }
    if( strstr(flags, "STRICTATIME") != NULL ) {
        res |= MS_STRICTATIME;
    }
    if( strstr(flags, "SYNCHRONOUS") != NULL ) {
        res |= MS_SYNCHRONOUS;
    }
    if( strstr(flags, "NOSYMFOLLOW") != NULL ) {
        res |= MS_NOSYMFOLLOW;
    }
    return res;
}

int parse_umountflags(char *flagnames) {
    int res = 0;
    char *flag = strtok(flagnames, ",");
    while( flag != NULL ) {
        if( strcmp(flag, "NOFLAG") == 0 ) {}
        else if( strcmp(flag, "FORCE") == 0 ) { res |= MNT_FORCE; }
        else if( strcmp(flag, "DETACH") == 0 ) { res |= MNT_DETACH; }
        else if( strcmp(flag, "EXPIRE") == 0 ) { res |= MNT_EXPIRE; }
        else if( strcmp(flag, "NOFOLLOW") == 0 ) { res |= UMOUNT_NOFOLLOW; }
        else {
            errno = EINVAL;
            return -1;
        };
        flag = strtok(NULL, ",");
    }
    return res;
}

int check_mountpoint(char *mnt) {
    /* Check that the running process is either owned by root, or
     *   * the mountpoint is a directory, and
     *   * the mountpoint is owned by the running process's owner, and
     *   * the mountpoint has u+rwx permissions.
     */
    struct stat mntstat = mntstat;

    if( getuid() == 0 ) {
        return 0;
    }

    if( lstat(mnt, &mntstat) == -1 ) {
        fprintf(stderr, "Could not stat %s, error %i\n", mnt, errno);
        return -1;
    }
    if(!S_ISDIR(mntstat.st_mode)) {
        errno = ENOTDIR;
        fprintf(stderr, "Mountpoint must be a directory.\n");
        return -1;
    }
    if(mntstat.st_uid != getuid()) {
        errno = EACCES;
        fprintf(stderr, "Mountpoint must be owned by invoking user.\n");
        return -1;
    }
    if(mntstat.st_mode && S_IRWXU != S_IRWXU ) {
        errno = EACCES;
        fprintf(stderr, "Mountpoint must have u+rwx permissions.\n");
        return -1;
    }
    fprintf(stderr, "Mount point looks good.\n");
    return 0;
}


int check_umountpoint(int uid, char *umnt) {
    /* Check if the unmount should be allowed based on the cookie in the
     * mnt_fsname field. Relies on PROCMOUNTS being ordered, the lowest
     * value corresponding to the topmost mount. Since the last entry
     * matching the mountpoint takes precedent, the check will only
     * succeed if the topmost mount has the right cookie.
     * The mountpoint must be given as an absolute path.
     */
    int res;

    char *fsprefix;
    int prefixlen;
    FILE *procmounts;
    struct mntent *nextmount;

    fsprefix = make_source_entry(uid, NULL);
    if( fsprefix == NULL ) {
        return -1;
    }
    prefixlen = strlen(fsprefix);

    procmounts = setmntent(PROCMOUNTS, "r");
    if( procmounts == NULL ) {
        free(fsprefix);
        errno = ENOMEM;
        return -1;
    }
    res = -1;
    errno = EPERM;
    nextmount = getmntent(procmounts);

    while( nextmount != NULL ) {
        if( strcmp(umnt, nextmount->mnt_dir) == 0 ) {
            if( strncmp(fsprefix, nextmount->mnt_fsname, prefixlen) == 0 ) {
                errno = 0;
                res = 0;
            }
            else {
                errno = EPERM;
                res = -1;
            }
        }
        nextmount = getmntent(procmounts);
    }
    endmntent(procmounts);
    free(fsprefix);
    return res;
    }

int do_umount(int uid, char *umnt, int flags) {
    /* Checks that the cookie in PROCMOUNTS matches and performs an unmount.
     * Takes an absolute path.
     *
     * TODO: This still allows unmounting a file system mounted below one that
     * passes the check by racing two instances of this function. Probably
     * needs to lock the directory below the mountpoint (or something similar).
     */
    int res = 1;
    if( uid != 0 && check_umountpoint(uid, umnt) == -1 ) {
        if( errno == EPERM ) {
            fprintf(stderr, "Umount not allowed.\n");
        } else {
            fprintf(stderr, "Umount permission check error: %i", errno);
        };
        res = -1;
    } else if( umount2(umnt, flags) == 0 ) {
        fprintf(stderr, "Umount successful.\n");
        res = 0;
    } else {
        fprintf(stderr, "Umount failed with error %i\n", errno);
        res = -1;
    };
    free(umnt);
    return res;
}

char *get_fusefd_start(char* options) {
    /* Find the position of the file descriptor option fd=X . If it does not
     * exist, return NULL.
     */
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

int get_fusefd(char* options) {
    /* Extract the file descriptor options's value from the options. If the
     * option is not set, return -1.
     */
    char *fdstart = get_fusefd_start(options);
    int fd;
    if( fdstart == NULL ) {
        errno = EINVAL;
        return -1;
    }
    errno = 0;
    fd = strtol(fdstart, NULL, 10);
    return (errno != 0) ? -1 : fd;
}

int check_fusefd(int fd) {
    /* Check that the file descriptor option is valid.
     */
    if( fcntl(fd, F_GETFD) ==  -1 ) {
        errno = EINVAL;
        return -1;
    }
    //TODO: Check that fd belongs to /dev/fuse ?
    return 0;
}

char *compose_mount_opts(char *options, int fd) {
    /* Concatenates the fd=X option with options string, returning a pointer
     * to a freshly allocated buffer. The caller is responsible for free()ing
     * that buffer.
     * Returns NULL if something goes wrong.
     */
    const int optlen = strlen(options);
    const int fdlen = snprintf(NULL, 0, "%i", fd);
    char *outopts;
    int res;

    if( optlen == 0 ) {
        outopts = calloc(fdlen + 4, sizeof(char));
        res = sprintf(outopts, "fd=%i", fd);
    }
    else {
        outopts = calloc(fdlen + 5 + optlen, sizeof(char));
        res = sprintf(outopts, "fd=%i,%s", fd, options);
    }
    if( res == -1 ) {
        free(outopts);
        return NULL;
    }
    return outopts;
}

int do_mount(char *mnt, char* options, int flags) {
    /* Run a mount operation on the file descriptor given.
     * TODO: Free fusesource
     */

    const char *fusefstype = FUSEFSTYPE;
    const char *fusesource = make_source_entry(getuid(), "somesuffix");

    int fd;

    if( check_capabilities() == -1 ) {
        fprintf(stderr, "Bad cababilities: Error %i\n", errno);
        return -1;
    }
    fd = get_fusefd(options);
    if( fd == -1 ) {
        fprintf(stderr, "Could not find or convert file descriptor in options\n");
        return -1;
    }
    if( check_fusefd(fd) == -1 ) {
        fprintf(stderr, "Bad file descriptor: %i\n", fd);
        return -1;
    }
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }
    if ( mount(fusesource, mnt, fusefstype, flags, options) == -1 ) {
        fprintf(stderr, "Failed to mount: Error %i\n", errno);
        return -1;
    }
    fprintf(stderr, "Mounted!\n");

    return 0;
}

int do_exec(char *mnt, char* options, int flags, char *execv[]) {
    /* Performs the following steps:
     *   * Open FUSEDEVICE
     *   * Add the file descriptor to the options
     *   * Perform a mount
     *   * Store the file descriptor in the FUSEFDVAR variable
     *   * Execute the arguments after the -- marker.
     * TODO: Free fusesource
     */

    const char *fusefstype = FUSEFSTYPE;
    const char *fusesource = make_source_entry(getuid(), "somesuffix");
    const char *fusefdvar = FUSEFDVAR;
    const char *fusedevice = FUSEDEVICE;

    int fd = get_fusefd(options);
    char *mntopts;

    if( check_capabilities() == -1 ) {
        fprintf(stderr, "Bad cababilities: Error %i\n", errno);
        return -1;
    }
    if ( fd != -1 ) {
        fprintf(stderr, "File descriptor found in exec mode options.\n");
        errno = EINVAL;
        return -1;
    }
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }
    fd = open(fusedevice, O_RDWR);
    mntopts = compose_mount_opts(options, fd);

    if( mntopts == NULL ) {
        fprintf(stderr, "Failed to compose mount options, errno %i\n", errno);
        return -1;
    }
    if( mount(fusesource, mnt, fusefstype, flags, mntopts) == -1 ) {
        fprintf(stderr, "Failed to mount: Error %i\n", errno);
        return -1;
    }
    fprintf(stderr, "Mounted!\n");

    //Can reuse mntopts buffer to prepare fd string
    if( sprintf(mntopts, "%i", fd) == -1 ) {
        fprintf(
            stderr
            , "Failed to write fd %i to environment: Could not reuse buffer\n"
            , fd
            );
        free(mntopts);
        return -1;
    }
    if( setenv(fusefdvar, mntopts, 1) == -1 ) {
        fprintf(
            stderr
            , "Failed to write fd %i to environment: Could not set %s\n"
            , fd, fusefdvar
            );
        free(mntopts);
        return -1;
    }
    free(mntopts);
    execvp(execv[0], execv);
    return -1; //To make the compiler happy
}
