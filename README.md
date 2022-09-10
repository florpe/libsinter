# libsinter

A more opinionated, less encompassing alternative to libfuse. In particular, libsinter is of the opinion that you should talk to the kernel module yourself.

With capabilities!

## Modes
    [ -m | --mount ] MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE
        Create mount at MOUNTPOINT using MOUNTCOOKIE for identification. The fd=X option in FUSEOPTIONS is required.
    [ -e | --exec ] MOUNTPOINT MOUNTFLAGS FUSEOPTIONS MOUNTCOOKIE -- EXEC [ ARG ... ]
        Create mount at MOUNTPOINT, store the file descriptor in $FUSEFD, then execute CMD [ ARG ... ] . The fd=X option in FUSEOPTIONS must not be set.
    [ -f | --files-exec ] FILE [ FILE ... ] -- EXEC [ ARG ... ]
        Read mount configurations line by line from FILEs until it encounters the -- argument, then execute CMD [ ARG ... ] .
        Each mount configuration line consists of a file descriptor or environment variable name followed by arguments as for the --exec option.
    [ -g | --files-noexec ] FILE [ FILE ... ]
        Like --files-noexec, but raises an error if it encounters the -- argument.
    [ -u | --unmount ] MOUNTPOINT UMOUNTFLAGS MOUNTCOOKIE
        Unmount the topmpost FUSE filesystem at MOUNTPOINT identified by MOUNTCOOKIE.
    [ -h | --help ]
        Show this help.
