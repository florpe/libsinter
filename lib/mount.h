
#include <sys/types.h>

int parse_mountflags(char *flags);
int parse_umountflags(char *flags);

int do_mount(char *mnt, char* options, int flags, char *tag);
int do_exec(char *mnt, char* options, int flags, char *fusefdvar, char *tag, char *execv[]);
int do_fromfile(char *terminus, int argc, char *argv[]);
int do_fromfile_exec(int argc, char *argv[]);
int do_singlefile(char *file);
int do_mountspec(char *envvar, char *mountpoint, char *mountflags, char *fuseopts, char *cookie);
int do_umount(int uid, char* mnt, int flags);

