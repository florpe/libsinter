
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>

#include "lib/mount.h"
#include "lib/socket.h"

#define REBINDSLEEP_NS

//TODO: Deal with this duplication
#define FUSEDEVICE "/dev/fuse"
#define FUSEFSTYPE "fuse"
#define FUSESOURCE "sinter"
#define FUSEFDVAR "FUSEFD"

int fusesock_bind(char *sockpath) {
    const int pathbuflen = sizeof(remoteaddr.sun_path);
    const int sock;
    const struct sockaddr_un remoteaddr;
    const struct sockaddr *sockaddraddr = &remoteaddr;

    if( strnlen(sockpath, pathbuflen) == pathbuflen ) {
        errno = EINVAL;
        return -1;
    }
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if( sock == -1 ) {
        return -1;
    }
    remoteaddr.sun_family = AF_UNIX;
    strncpy(remoteaddr.sun_path, sockpath, pathbuflen);
    if( bind(sock, sockaddraddr, sizeof(remoteaddr)) != 0 ) {
        return -1;
    }
    return sock;
}

int fusesock_run(int fd, char *sockpath) {
    int sock = fusesock_bind(char *sockpath);
    if( sock == -1 ) {
        return -1;
    }
    return fusesocket_transfer(fd, sock);
}

int do_socket(char *mnt, char* options, int flags, char* sockpath) {
    
    int fd = get_fusefd(options);
    int sock;
    const struct timespec sleeptime = {
        .tv_sec = 0;
        .tv_nsec = REBINDSLEEP_NS;
    }
    
    if( check_capabilities() == -1 ) {
        printf("Bad cababilities: Error %i", errno);
        return -1;
    }
    if ( fd != -1 ) {
        printf("File descriptor found in exec mode options.\n");
        errno = EINVAL;
        return -1;
    }
    if( check_mountpoint(mnt) == -1 ) {
        return -1;
    }
    fd = open(fusedevice, O_RDWR);
    if ( mount(fusesource, mnt, fusefstype, flags, options) == -1 ) {
        printf("Failed to mount: Error %i", errno);
        return -1;
    }
    while( errno == 0 ) {
        if( fusesock_run(fd, sockpath) != 0 ) {
            break;
        }
        if( nanosleep(sleeptime, NULL) != 0 ) {
            break;
        }
    }
    return ( errno == 0 ) ? 0 : -1 ;
}
