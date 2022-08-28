# libsinter

A more opinionated, less encompassing alternative to libfuse. With capabilities!

## Modes
    -m MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE
        Create mount at MOUNTPOINT using MOUNTCOOKIE for identification. The fd=X option in FUSEOPTIONS is required.
    -e MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE -- CMD [ ARG ... ]
        Create mount at MOUNTPOINT, store the file descriptor in $FUSEFD, then execute CMD [ ARG ... ] . The fd=X option in FUSEOPTIONS must not be set.
    -u MOUNTPOINT UMOUNTFLAGS MOUNTCOOKIE
        Unmount the topmpost FUSE filesystem at MOUNTPOINT identified by MOUNTCOOKIE.
    -h, --help
        Show this help.
