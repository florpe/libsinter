
#include <sys/types.h>

int check_capabilities();
int parse_mountflags(char *flags);
int parse_umountflags(char *flags);
char *make_source_entry(int uid, char *suffix);

int check_mountpoint(char *mnt);
int check_umountpoint(int uid, char *mnt);
char *compose_mount_opts(char *options, int fd);
char *get_fusefd_start(char *options);
int get_fusefd(char *options);
int check_fusefd(int fd);
int do_mount(char *mnt, char* options, int flags, char *tag);
int do_exec(char *mnt, char* options, int flags, char *fusefdvar, char *tag, char *execv[]);
int do_fromfile(char *terminus, int argc, char *argv[]);
int do_fromfile_exec(int argc, char *argv[]);
int do_singlefile(char *file);
int do_mountspec(char *envvar, char *mountpoint, char *mountflags, char *fuseopts, char *cookie);
int do_umount(int uid, char* mnt, int flags);

