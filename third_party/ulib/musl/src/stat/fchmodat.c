#include "syscall.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

void __procfdname(char*, unsigned);

int fchmodat(int fd, const char* path, mode_t mode, int flag) {
    if (!flag)
        return syscall(SYS_fchmodat, fd, path, mode, flag);

    if (flag != AT_SYMLINK_NOFOLLOW)
        return __syscall_ret(-EINVAL);

    struct stat st;
    int ret, fd2;
    char proc[15 + 3 * sizeof(int)];

    if ((ret = fstatat(fd, path, &st, flag)) != 0)
        return ret;
    if (S_ISLNK(st.st_mode))
        return __syscall_ret(-EOPNOTSUPP);

    if ((fd2 = openat(fd, path,
                      O_RDONLY | O_PATH | O_NOFOLLOW | O_NOCTTY | O_CLOEXEC)) < 0) {
        if (errno == ELOOP)
            errno = EOPNOTSUPP;
        return -1;
    }

    __procfdname(proc, fd2);
    ret = fstatat(AT_FDCWD, proc, &st, 0);
    if (!ret) {
        if (S_ISLNK(st.st_mode))
            ret = -EOPNOTSUPP;
        else
            ret = __syscall(SYS_fchmodat, AT_FDCWD, proc, mode);
    }

    close(fd2);
    return __syscall_ret(ret);
}