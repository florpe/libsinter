
#include <errno.h>
#include <fcntl.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include "sinter.h"

#define PROCMOUNTS "/proc/mounts"
#define FUSEFSTYPE "fuse"
#define FUSESOURCE_FMT_PREFIX "sinter-0x%.4X-"
#define FUSESOURCE_FMT_ENTIRE "sinter-0x%.4X-%s"

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
//        fprintf(stderr, "Could not stat %s, error %i\n", mnt, errno);
        return -1;
    }
    if(!S_ISDIR(mntstat.st_mode)) {
        errno = ENOTDIR;
//        fprintf(stderr, "Mountpoint must be a directory.\n");
        return -1;
    }
    if(mntstat.st_uid != getuid()) {
        errno = EACCES;
//        fprintf(stderr, "Mountpoint must be owned by invoking user.\n");
        return -1;
    }
    if(mntstat.st_mode && S_IRWXU != S_IRWXU ) {
        errno = EACCES;
//        fprintf(stderr, "Mountpoint must have u+rwx permissions.\n");
        return -1;
    }
//    fprintf(stderr, "Mountpoint looks good.\n");
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

char *compose_fuseopts(int fd, char *fuseopts) {
    /* Concatenates the fd=X option to the fuseopts string, returning
     * a pointer to a buffer that must be deallocated by the caller.
     *
     * Returns NULL if something goes wrong.
     */
    const int optlen = strlen(fuseopts);
    const int fdlen = snprintf(NULL, 0, "%i", fd);
    char *outopts;
    int res;

    if( optlen == 0 ) {
        outopts = calloc(fdlen + 4, sizeof(char));
        res = sprintf(outopts, "fd=%i", fd);
    }
    else {
        outopts = calloc(fdlen + 5 + optlen, sizeof(char));
        res = sprintf(outopts, "fd=%i,%s", fd, fuseopts);
    }
    if( res == -1 ) {
        free(outopts);
        return NULL;
    }
    return outopts;
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

int get_fusefd(char *options) {
    /* Extract the file descriptor options's value from the options. If the
     * option is not set, return -1.
     */
    char *fdstart = get_fusefd_start(options);
    int fd;
    if( fdstart == NULL ) {
        return -1;
    }
    errno = 0;
    fd = strtol(fdstart, NULL, 10); //TODO: Get rid of the integer constant, maybe?
    return (errno == 0) ? fd : -1;
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

int fusemount(char *fusesource, char *mnt, int mountflags, char *fuseopts) {
    const char *fusefstype = FUSEFSTYPE;
    int res;
    res = mount(fusesource, mnt, fusefstype, mountflags, fuseopts);
    return res;
}

int fusemount_fd(char *fusesource, char *mnt, int mountflags, char *fuseopts, int fd) {
    char *fuseopts_full;
    int res;

    if( NULL != get_fusefd_start(fuseopts) ) {
        errno = EINVAL;
        return -1;
    }
    fuseopts_full = compose_fuseopts(fd, fuseopts);
    if( NULL == fuseopts_full ) {
        return -1;
    }
    res = fusemount(fusesource, mnt, mountflags, fuseopts_full);
    free(fuseopts_full);
    return res;
}

int setenv_fd(char *envvar, int fd) {
    char *varcontent = calloc(snprintf(NULL, 0, "%i", fd), sizeof(char));
    int res = sprintf(varcontent, "%i", fd);
    if( res != -1 ) {
        res = setenv(envvar, varcontent, 1);
    }
    free(varcontent);
    return res;
}
