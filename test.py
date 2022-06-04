
from os import open as osopen, readv as osreadv, O_RDWR, O_DIRECT
from subprocess import run as srun

FUSE_MIN_READ_BUFFER = 8192

def main():
    fusefd = osopen('/dev/fuse', O_RDWR)
    options = f'default_permissions,fd={fusefd},rootmode=40000,user_id=1000,group_id=100'
    print(options)
    runres = srun(
        [
            './foo/sinter'
            , '-m'
            , 'mnt'
            , 'RDONLY'
            , options
            ]
        , pass_fds=(fusefd,)
        , capture_output=True
        , text=True
        )
    print(runres.stdout)
    print(fusefd)
    buf = bytearray(FUSE_MIN_READ_BUFFER)
    readcount = osreadv(fusefd, (buf,))
    #TODO: Actually do some fuse operations

if __name__ == "__main__":
    main()

