# Libsinter

A more opinionated, less encompassing alternative to libfuse. With capabilities!

## Modes

    -m MOUNTPOINT MOUNTFLAGS OPTIONS
        Mounting mode. The fd=X option is required.
    -e MOUNTPOINT MOUNTFLAGS OPTIONS -- CMD [ CMD ... ]
        Exec mode. Execute everything after -- with regular permissions. The fs=X options must not be set.
    -u MOUNTPOINT
        Unmount mode.