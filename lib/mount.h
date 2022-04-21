
#include <sys/types.h>

int check_mountpoint(char *mnt);
int check_fusefd(char *mnt);
int do_mount(char *mnt, char* options, int flags);

