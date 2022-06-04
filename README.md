# libsinter

A more opinionated, less encompassing alternative to libfuse. With capabilities!

## Modes

    -m MOUNTPOINT FLAGS OPTIONS
        Mounting mode. The fd=X option is required.
    -e MOUNTPOINT FLAGS OPTIONS -- CMD [ CMD ... ]
        Exec mode. Execute everything after -- with regular permissions. The fd=X options must not be set.
    -u MOUNTPOINT FLAGS
        Unmount mode.
