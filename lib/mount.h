
#include <sys/types.h>

int check_capabilities();
int check_mountpoint(char *mnt);
int check_fusefd(char *mnt);
int do_mount(char *mnt, char* options, int flags);
int do_exec(char *mnt, char* options, int flags, char *execv[]);

