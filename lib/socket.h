
#include <sys/types.h>

int fusesock_transfer(int fd, int sock);
int fusesock_bind(char *sockpath);
int fusesock_run(int fd, char* sockpath);
int do_socket(char *mnt, char* options, int flags, char* sockpath);

