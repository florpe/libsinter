
char *make_source_entry(int uid, char *suffix);

int check_capabilities();
int check_mountpoint(char *mnt);
int check_umountpoint(int uid, char *mnt);
int check_fusefd(int fd);

int get_fusefd(char *options);
char *get_fusefd_start(char *options);
char *compose_fuseopts(int fd, char *fuseopts);

int fusemount(char *fusesource, char *mnt, int mountflags, char *fuseopts);
int fusemount_fd(char *fusesource, char *mnt, int mountflags, char *fuseopts, int fd);

int setenv_fd(char *envvar, int fd);
