//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include <fcntl.h>

#include "types.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "string.h"
#include "console.h"
#include "log.h"
#include "fs.h"
#include "file.h"

#include "syscall.h"

struct iovec {
    void  *iov_base;    /* Starting address. */
    size_t iov_len;     /* Number of bytes to transfer. */
};

/*
 * Fetch the nth word-sized system call argument as a file descriptor
 * and return both the descriptor and the corresponding struct file.
 */
static int
argfd(int n, int64_t *pfd, struct file **pf)
{
    int64_t fd;
    struct file *f;

    if (argint(n, &fd) < 0)
        return -1;
    if (fd < 0 || fd >= NOFILE || (f = thisproc()->ofile[fd]) == 0)
        return -1;
    if (pfd)
        *pfd = fd;
    if (pf)
        *pf = f;
    return 0;
}

/*
 * Allocate a file descriptor for the given file.
 * Takes over file reference from caller on success.
 */
static int
fdalloc(struct file *f)
{
    /* TODO: Your code here. */
    for (int i = 0; i < NOFILE; i++) {
        if (thisproc()->ofile[i] == 0) {
            thisproc()->ofile[i] = f;
            return i;
        } 
    }
    return -1;
}

int
sys_dup()
{
    /* TODO: Your code here. */
    struct file* f;
    if (argfd(0, 0, &f) < 0) {
        return -1;
    }

    int fd = fdalloc(f);
    if (fd < 0) {
        return -1;
    }
    filedup(f);
    return fd;
}

ssize_t
sys_read()
{
    /* TODO: Your code here. */
    struct file* f;
    char* addr;
    ssize_t n;

    if (argfd(0, 0, &f) < 0 ||
        argint(2, &n) < 0 ||
        argptr(1, &addr, n) < 0) {
        return -1;
    } 
    return fileread(f, addr, n);
}

ssize_t
sys_write()
{
    /* TODO: Your code here. */
    struct file* f;
    char* addr;
    ssize_t n;

    if (argfd(0, 0, &f) < 0 ||
        argint(2, &n) < 0 ||
        argptr(1, &addr, n) < 0) {
        return -1;
    } 
    return filewrite(f, addr, n);
}


ssize_t
sys_writev()
{
    /* TODO: Your code here. */

    /* Example code.
     *
     * ```
     * struct file *f;
     * int64_t fd, iovcnt;
     * struct iovec *iov, *p;
     * if (argfd(0, &fd, &f) < 0 ||
     *     argint(2, &iovcnt) < 0 ||
     *     argptr(1, &iov, iovcnt * sizeof(struct iovec)) < 0) {
     *     return -1;
     * }
     *
     * size_t tot = 0;
     * for (p = iov; p < iov + iovcnt; p++) {
     *     // in_user(p, n) checks if va [p, p+n) lies in user address space.
     *     if (!in_user(p->iov_base, p->iov_len))
     *          return -1;
     *     tot += filewrite(f, p->iov_base, p->iov_len);
     * }
     * return tot;
     * ```
     */

    struct file *f;
    int64_t fd, iovcnt;
    struct iovec *iov;
    if (argfd(0, &fd, &f) < 0 ||
        argint(2, &iovcnt) < 0 ||
        argptr(1, &iov, iovcnt * sizeof(struct iovec)) < 0) {
        return -1;
    }
    size_t tot = 0;
    for (struct iovec* p = iov; p < iov + iovcnt; p++) {
        if (0) {
            return -1;
        } 
        tot += filewrite(f, p->iov_base, p->iov_len);
    }
    return tot;
}

int
sys_close()
{
    /* TODO: Your code here. */
    struct file *f;
    int fd;
    
    if(argfd(0, &fd, &f) < 0) {
        return -1;
    }

    thisproc()->ofile[fd] = 0;
    fileclose(f);

    return 0;
}

int
sys_fstat()
{
    /* TODO: Your code here. */
    struct file *f;
    struct stat *st;

    if (argfd(0, 0, &f) < 0 ||
        argptr(1, (void*)&st, sizeof(*st)) < 0) {
        return -1;
    }

    return filestat(f, st);
}

int
sys_fstatat()
{
    int64_t dirfd, flags;
    char *path;
    struct stat *st;

    if (argint(0, &dirfd) < 0 ||
        argstr(1, &path) < 0 ||
        argptr(2, (void *)&st, sizeof(*st)) < 0 ||
        argint(3, &flags) < 0)
        return -1;

    if (dirfd != AT_FDCWD) {
        cprintf("sys_fstatat: dirfd unimplemented\n");
        return -1;
    }
    if (flags != 0) {
        cprintf("sys_fstatat: flags unimplemented\n");
        return -1;
    }

    struct inode *ip;
    begin_op();
    if ((ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    stati(ip, st);
    iunlockput(ip);
    end_op();

    return 0;
}

struct inode *
create(char *path, short type, short major, short minor)
{
    /* TODO: Your code here. */
    uint32_t off;
    struct inode *ip, *dp;
    char name[DIRSIZ] = {0};

    if((dp = nameiparent(path, name)) == 0) {
        return 0;
    }

    ilock(dp);

    if((ip = dirlookup(dp, name, (size_t*)&off)) != 0){
        iunlockput(dp);
        ilock(ip);

        if(type == T_FILE && ip->type == T_FILE) {
            return ip;
        }

        iunlockput(ip);

        return 0;
    }

    if((ip = ialloc(dp->dev, type)) == 0) {
        panic("create: ialloc");
    }

    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);

    if(type == T_DIR){  // Create . and .. entries.
        dp->nlink++;  // for ".."
        iupdate(dp);

        // No ip->nlink++ for ".": avoid cyclic ref count.
        if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0) {
            panic("create dots");
        }
    }

    if(dirlink(dp, name, ip->inum) < 0) {
        panic("create: dirlink");
    }

    iunlockput(dp);

    return ip;
}

int
sys_openat()
{
    char *path;
    int64_t dirfd, fd, omode;
    struct file *f;
    struct inode *ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &omode) < 0)
        return -1;

    // cprintf("%lld, %s, %lld\n", dirfd, path, omode);
    if (dirfd != AT_FDCWD) {
        cprintf("sys_openat: dirfd unimplemented\n");
        return -1;
    }
    if ((omode & O_LARGEFILE) == 0) {
        cprintf("sys_openat: expect O_LARGEFILE in open flags\n");
        return -1;
    }

    begin_op();
    if (omode & O_CREAT) {
        // FIXME: Support acl mode.
        ip = create(path, T_FILE, 0, 0);
        if (ip == 0) {
            end_op();
            return -1;
        }
    } else {
        if ((ip = namei(path)) == 0) {
            end_op();
            return -1;
        }
        ilock(ip);
        if (ip->type == T_DIR && omode != (O_RDONLY | O_LARGEFILE)) {
            iunlockput(ip);
            end_op();
            return -1;
        }
    }

    if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
        if (f)
            fileclose(f);
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    end_op();

    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
    return fd;
}

int
sys_mkdirat()
{
    int64_t dirfd, mode;
    char *path;
    struct inode *ip;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &mode) < 0)
        return -1;
    if (dirfd != AT_FDCWD) {
        cprintf("sys_mkdirat: dirfd unimplemented\n");
        return -1;
    }
    if (mode != 0) {
        cprintf("sys_mkdirat: mode unimplemented\n");
        return -1;
    }

    begin_op();
    if ((ip = create(path, T_DIR, 0, 0)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int
sys_mknodat()
{
    struct inode *ip;
    char *path;
    int64_t dirfd, major, minor;

    if (argint(0, &dirfd) < 0 || argstr(1, &path) < 0 || argint(2, &major) < 0 || argint(3, &minor))
        return -1;

    if (dirfd != AT_FDCWD) {
        cprintf("sys_mknodat: dirfd unimplemented\n");
        return -1;
    }
    cprintf("mknodat: path '%s', major:minor %d:%d\n", path, major, minor);

    begin_op();
    if((ip = create(path, T_DEV, major, minor)) == 0) {
        end_op();
        return -1;
    }
    iunlockput(ip);
    end_op();
    return 0;
}

int
sys_chdir()
{
    char *path;
    struct inode *ip;
    struct proc *curproc = thisproc();
  
    begin_op();
    if (argstr(0, &path) < 0 || (ip = namei(path)) == 0) {
        end_op();
        return -1;
    }
    ilock(ip);
    if (ip->type != T_DIR) {
        iunlockput(ip);
        end_op();
        return -1;
    }
    iunlock(ip);
    iput(curproc->cwd);
    end_op();
    curproc->cwd = ip;
    return 0;
}

int
sys_exec()
{
    /* TODO: Your code here. */
    char* path;
    char* argv[1 << 6];
    // char* envp[1 << 6];
    uint64_t uargv;
    if (argstr(0, &path) < 0 ||
        argint(1, (long*)&uargv) < 0) {
        return -1;
    }
    memset(argv, 0, sizeof(argv));
    uint64_t uarg;
    for (int i = 0; i <= (1 << 6); i++) {
        if (i == (1 << 6)) {
            return -1;
        }
        if (fetchint(uargv + (i << 3), (long*)&uarg) < 0) {
            return -1;
            
        }
        if (uarg == 0) {
            argv[i] = 0;
            break;
        }

        if (fetchstr(uarg, &argv[i]) < 0) {
            return -1;
        } 
    }
    /* for (int i = 0; i <= (1 << 6); i++) {
        if (i == (1 << 6)) {
            return -1;
        }
        if (fetchint(uargv + (i << 3), (long*)&uarg) < 0) {
            return -1;
            
        }
        if (uarg == 0) {
            envp[i] = 0;
            break;
        }

        if (fetchstr(uarg, &envp[i]) < 0) {
            return -1;
        } 
    } */
    

    return execve(path, argv, (char**)0);
}

