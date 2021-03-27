#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/seek.h>
#include <lib.h>
#include <uio.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <vfs.h>
#include <vnode.h>
#include <file.h>
#include <syscall.h>
#include <copyinout.h>

/*
 * Add your file-related functions here ...
 */

int sys_open(const char *filename, int flags, mode_t mode, int *retval) {

    // check filename
    char path[NAME_MAX];
    size_t got;
    int copyerr = copyinstr((const_userptr_t)filename, path, 
                                    NAME_MAX, &got);
    if (copyerr) {
        return copyerr;
    }

        int fd = -1;
    // get free file descripter index in open file table
    for (int i = 0; i < OPEN_MAX; i++) {
        if (curproc->fd_table[i] == -1) {
            fd = i;
            break;
        }
    }
    // file descriptor table is full
    if (fd == -1) {
        return EMFILE;
    }
    
    lock_acquire(of_table_lock);

    int of_index = -1;
    for (int i = 0; i < OPEN_MAX; i++) {
        if (of_table[i]->flags == -1) {
            of_index = i;

            break;
        }
    }

    if (of_index == -1) {
        lock_release(of_table_lock);
        return ENFILE;
    }

    struct vnode *vn;
    // open file and put data in vnode
    int fderr = vfs_open(path, flags, mode, &vn);
    if (fderr) {
        lock_release(of_table_lock);
        return fderr;
    }

    /*  
     * Allocate data from vfs opened file into 
     * open file node (index is same as fd_table[fd]) 
     */
    curproc->fd_table[fd] = of_index;
    of_table[of_index]->flags = flags;
    of_table[of_index]->fp = 0;
    of_table[of_index]->refcount = 0;
    of_table[of_index]->vn = vn;
    lock_release(of_table_lock);
    *retval = fd;
    return 0;
}

int sys_close (int fd) {
    // unopened fd
    if (curproc->fd_table[fd] == -1) {
        return EBADF;
    }

    lock_acquire(of_table_lock);

    // if last fd pointing to this of, vfs close
    of_node *of = of_table[curproc->fd_table[fd]];
    if (of->refcount == 1) {
        vfs_close(of->vn);
        of_table[curproc->fd_table[fd]]->flags = -1;
        of_table[curproc->fd_table[fd]]->fp = -1;
        of_table[curproc->fd_table[fd]]->refcount = -1;
        of_table[curproc->fd_table[fd]]->vn = NULL;
    }
    of_table[curproc->fd_table[fd]]->refcount--;
    curproc->fd_table[fd] = -1;
    lock_release(of_table_lock);

    return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *retval) {
    // unopened fd
    if (curproc->fd_table[fd] == -1) {
        return EBADF;
    }

    lock_acquire(of_table_lock);
    // initialise uio using open file
    struct iovec myiov;
    struct uio myuio;
    of_node *of = of_table[curproc->fd_table[fd]];

    if (of->flags != O_RDONLY || (of->flags & O_RDWR) != of->flags) {
        lock_release(of_table_lock);
        return EBADF;
    }

    uio_kinit(&myiov, &myuio, (void *)buf, count, of->fp, UIO_READ);


    int err = VOP_READ(of->vn, &myuio);
    if (err) {
        lock_release(of_table_lock);
        return err;
    }

    *retval = count - myuio.uio_resid;
    
    lock_release(of_table_lock);

    return 0;
}

int sys_write (int fd, const void *buf, size_t count, ssize_t *retval) {
    // unopened fd
    if (curproc->fd_table[fd] == -1) {
        return EBADF;
    }

    lock_acquire(of_table_lock);
    // initialise uio using open file
    struct iovec myiov;
    struct uio myuio;
    of_node *of = of_table[curproc->fd_table[fd]];

    if (of->flags != O_WRONLY || (of->flags & O_RDWR) != of->flags) {
        lock_release(of_table_lock);
        return EBADF;
    }

    uio_kinit(&myiov, &myuio, (void *)buf, count, of->fp, UIO_WRITE);


    int err = VOP_WRITE(of->vn, &myuio);
    if (err) {
        lock_release(of_table_lock);
        return err;
    }

    *retval = count - myuio.uio_resid;
    
    lock_release(of_table_lock);

    return 0;
}



int sys_lseek(int fd, off_t offset, int whence, off_t *retval64) {
    // unopened fd
    if (curproc->fd_table[fd] == -1) {
        return EBADF;
    }

    if (whence < 0 || whence > 2) {
        return EINVAL;
    }

    lock_acquire(of_table_lock);

    of_node *of = of_table[curproc->fd_table[fd]];

    // Checks if file is seekable
    if(!VOP_ISSEEKABLE(of->vn)){
        lock_release(of_table_lock);
        return ESPIPE;
    }

    // Put offset into retval64
    if (whence == SEEK_SET) {
        if (offset < 0) {
            lock_release(of_table_lock);
            return EINVAL;
        }
        of->fp = offset;
        *retval64 = of->fp;
        lock_release(of_table_lock);
        return 0;
    }
    else if (whence == SEEK_CUR) {
        if ((of->fp + offset) < 0) {
            lock_release(of_table_lock);
            return EINVAL;
        }
        of->fp += offset;
        *retval64 = of->fp;
        lock_release(of_table_lock);
        return 0;
    }
    else {
        struct stat file_stat;
        int err = VOP_STAT(of->vn, &file_stat);
        if (err) {
            return err;
        }
        
        if ((of->fp + file_stat.st_size) < 0) {
            lock_release(of_table_lock);
            return EINVAL;
        }
        of->fp = file_stat.st_size + offset;
        *retval64 = of->fp;
        lock_release(of_table_lock);
        return 0;
    }
}

int sys_dup2(int oldfd, int newfd) {
    // old fd not valid
    if (curproc->fd_table[oldfd] == -1 || newfd < 0 || newfd > OPEN_MAX) {
        return EBADF;
    }
    // dup2 same fd
    if (oldfd == newfd) {
        return 0;
    }
    // new fd points to open file already, close it
    if (curproc->fd_table[newfd] != -1) {
        int err = sys_close(newfd);
        if (err) {
            return err;
        }
    }
    lock_acquire(of_table_lock);

    of_node *old_of = of_table[curproc->fd_table[oldfd]];
    
    curproc->fd_table[newfd] = curproc->fd_table[oldfd];
    old_of->refcount++;

    lock_release(of_table_lock);

    return 0;
}