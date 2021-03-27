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
    int filename_check = copyinstr((const_userptr_t)filename, path, 
                                    NAME_MAX, &got);
    if (filename_check) {
        return filename_check;
    }

    // check flags
    int all_flags = O_ACCMODE | O_NOCTTY | O_APPEND | O_TRUNC | O_EXCL | O_CREAT;
    if ((flags & all_flags) != flags) {
        return EINVAL;
    }

    lock_acquire(of_table_lock);
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
        lock_release(of_table_lock);
        return EMFILE;
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
     * open file node (index is same as fd) 
     */
    for (int i = 0; i < OPEN_MAX; i++) {
        if (of_table[i]->flags == -1) {
            curproc->fd_table[fd] = i;
            of_table[i]->flags = flags;
            of_table[i]->fp = 0;
            of_table[i]->refcount = 0;
            of_table[i]->vn = vn;
        }
    }

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
    of_node *of = of_table[curproc->fd_table[fd]];
    vfs_close(of->vn);
    of_table[curproc->fd_table[fd]]->flags = -1;
    of_table[curproc->fd_table[fd]]->fp = -1;
    of_table[curproc->fd_table[fd]]->refcount = -1;
    of_table[curproc->fd_table[fd]]->vn = NULL;
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



// off_t lseek(int fd, off_t offset, int whence) {

// }