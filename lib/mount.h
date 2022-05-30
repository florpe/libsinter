
#include <sys/types.h>

int check_capabilities();
int parse_mountflags(char *flags);

int check_mountpoint(char *mnt);
char *compose_mount_opts(char *options, int fd);
char *get_fusefd_start(char *options);
int get_fusefd(char *options);
int check_fusefd(int fd);
int do_mount(char *mnt, char* options, int flags);
int do_exec(char *mnt, char* options, int flags, char *execv[]);

