
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
#include "sinter.h"

#define FUSEDEVICE "/dev/fuse"

#define FUSEFSTYPE "fuse"

#define WHITESPACE " \n\t"

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

int do_mount(char *mnt, char* options, int flags, char *cookie) {
    /* Run a mount operation on the file descriptor given.
     */

    char *fusesource;

    int fd;

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
    fusesource = make_source_entry(getuid(), cookie);
    if( fusesource == NULL ) {
        fprintf(stderr, "Could not create mount cookie: Error %i\n", errno);
        return -1;
    };
    if ( fusemount(fusesource, mnt, flags, options) == -1 ) {
        fprintf(stderr, "Failed to mount: Error %i, %s, %s, %i, %s\n", errno, fusesource, mnt, flags, options);
        free(fusesource);
        return -1;
    }
    free(fusesource);

    return 0;
}

int do_exec(char *mnt, char* options, int flags, char *fusefdvar, char *cookie, char *execv[]) {
    /* Performs the following steps:
     *   * Open FUSEDEVICE
     *   * Add the file descriptor to the options
     *   * Perform a mount
     *   * Store the file descriptor in the fusefdvar variable
     *   * Execute the arguments after the -- marker.
     * TODO: Clean up resource management.
     */

    char *fusesource;
    const char *fusedevice = FUSEDEVICE;

    int fd;

    if ( get_fusefd_start(options) != NULL ) {
        fprintf(stderr, "File descriptor found in exec mode options.\n");
        errno = EINVAL;
        return -1;
    }
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }


    fusesource = make_source_entry(getuid(), cookie);
    if( fusesource == NULL ) {
        fprintf(stderr, "Could not create mount source: Error %i\n", errno);
        return -1;
    }
    fd = open(fusedevice, O_RDWR);
    if( fusemount_fd(fusesource, mnt, flags, options, fd) == -1 ) {
        fprintf(stderr, "Failed to mount: Error %i, %s, %s, %i, %s\n", errno, fusesource, mnt, flags, options);
        free(fusesource);
        close(fd);
        return -1;
    }
    free(fusesource);

    if( fusefdvar != NULL && setenv_fd(fusefdvar, fd) == -1 ) {
        fprintf(
            stderr
            , "Failed to write fd %i to environment\n"
            , fd
            );
        close(fd);
        return -1;
    }

    //No close(fd) here!
    if( execv != NULL ) {
        execvp(execv[0], execv);
    }
    return 0;
}

int do_fromfiles (char *terminus, int argc, char *argv[]) {
    /* Processes argv[] entries as files containing
     * a mount spec, terminating when the entry is equal to
     * terminus. If terminus is NULL, it processes all entries.
     *
     * Returns the number of arguments consumed.
     */
    int count = 0;
    errno = 0;
    for( count = 0; count < argc; count++ ) {
        if( terminus != NULL && strcmp(argv[count], terminus) == 0 ) {
            break;
        }
        errno = do_singlefile(argv[count]);
        if( errno != 0 ) {
            return -1;
        }
    }
    return count;
}

int do_fromfiles_exec (int argc, char *argv[]) {
    int count = do_fromfiles("--", argc, argv);
    if( count == -1 ) {
        return -1;
    }
    if( count == argc ) {
        return 0;
    }
    if( count + 1 == argc ) {
        fprintf(stderr, "No command specified.\n");
        errno = EINVAL;
        return -1;
    }
    execvp(argv[count + 1], argv + count + 1);
    return -1; //To make the compiler happy
}

int do_fromfiles_noexec (int argc, char *argv[]) {
    int count = do_fromfiles("--", argc, argv);
    if( count == -1 ) {
        return -1;
    }
    if( count == argc ) {
        return 0;
    }
    fprintf(stderr, "Command separator -- found in noexec mode.\n");
    errno = EINVAL;
    return -1;
}

int do_singlefile (char *file) {
    FILE *fp;
    int res;
    char *line = NULL;
    size_t linelen = 0;
    char *saveptr = NULL;

    char *envvar;
    char *mountpoint;
    char *mountflags;
    char *fuseopts;
    char *cookie;

    if( strcmp(file, "-") == 0 ) {
        fp = stdin;
    } else {
        fp = fopen(file, "r");
    }
    if( fp == NULL ) {
        return -1;
    }
    res = getc(fp);
    res = ungetc(res, fp);
    if( res == EOF ) {
        fprintf(stderr, "Could not getc, ungetc on %s\n", file);
        return 0;
    }
    if( res == 0 ) {
        fprintf(stderr, "0x00-spec not implemented: %s\n", file);
        errno = ENOSYS;
        return -1;
    }
    while( getline(&line, &linelen, fp) != -1 ) {
        envvar = strtok_r(line, WHITESPACE, &saveptr);
        mountpoint = strtok_r(NULL, WHITESPACE, &saveptr);
        mountflags = strtok_r(NULL, WHITESPACE, &saveptr);
        fuseopts = strtok_r(NULL, WHITESPACE, &saveptr);
        cookie = strtok_r(NULL, WHITESPACE, &saveptr);
        if(
            envvar == NULL
            && mountpoint == NULL
            && mountflags == NULL
            && fuseopts == NULL
            && cookie == NULL
        ) {
            continue;
        }
        if(
            envvar == NULL
            || mountpoint == NULL
            || mountflags == NULL
            || fuseopts == NULL
            || cookie == NULL
        ) {
            free(line);
            line = NULL;
            saveptr = NULL;

            errno = EINVAL;
            return -1;
        }
        if( *envvar != '#' ) {
            if( do_mountspec(envvar, mountpoint, mountflags, fuseopts, cookie) == -1 ) {
                break;
            }
        }
        free(line);
        line = NULL;
        saveptr = NULL;
    }
    free(line);
    if( fp != stdin ) {
        fclose(fp);
    }
    return ( errno == 0 ? 0 : -1 ) ;
}

int do_mountspec(char *envvar, char *mountpoint, char *mountflags, char *fuseopts, char *cookie) {
    const char *fusedevice = FUSEDEVICE;
    int fd;
    char *endptr = NULL;
    char *fusesource;
    int mntflgs = parse_mountflags(mountflags);
    int res;
    if( check_mountpoint(mountpoint) == -1 ) {
        fprintf(stderr, "Bad mounpoint%s, error %i\n", mountpoint, errno);
        return -1;
    }

    if( get_fusefd_start(fuseopts) != NULL ) {
        fprintf(stderr, "File descriptor specified in options for mounpoint%s\n", mountpoint);
        errno = EINVAL;
        return -1;
    }
    errno = 0;
    fd = strtol(envvar, &endptr, 0);
    if( errno == ERANGE ) {
        fprintf(stderr, "File descriptor %s too big at mountpoint %s\n", envvar, mountpoint);
        return -1;
    }

    fusesource = make_source_entry(getuid(), cookie);
    if( fusesource == NULL ) {
        fprintf(stderr, "Could not create mount source for %s: Error %i\n", mountpoint, errno);
        return -1;
    }

    if( *endptr != '\0' ) {
        fd = open(fusedevice, O_RDWR);
    }
//        return do_exec(mountpoint, fuseopts, mntflgs, NULL, cookie, NULL);
    res = fusemount_fd(fusesource, mountpoint, mntflgs, fuseopts, fd);
    if( res != 0 ) {
        fprintf(stderr, "Could not mount %s with options %s: Error %i\n", mountpoint, fuseopts, errno);
    }
    if ( *endptr != '\0' ) {
        if( res != 0 ) {
            close(fd);
        }
        else {
            setenv_fd(envvar, fd);
        }
    }
    free(fusesource);
    return res;
}
