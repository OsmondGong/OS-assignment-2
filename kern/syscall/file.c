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
of_table all_of[OPEN_MAX];


int sys_open(const char *filename, int flags) {
    
    // get trapframe
    struct trapframe *tf;
    syscall(tf);

    // check filename
    char path[NAME_MAX];
    size_t got;
    int filename_check = copyinstr((const_userptr_t)tf->tf_a0, path, 
                                    NAME_MAX, &got);
    if (filename_check != 0) {
        return filename_check;
    }

    // check flags
    int all_flags = O_ACCMODE | O_NOCTTY | O_APPEND | O_TRUNC | O_EXCL | O_CREAT;
    if ((flags & all_flags) != flags) {
        return EINVAL;
    }
    
    int fd = -1;
    // get free file descripter index in file descriptor table
    for (int i = 0; i < FD_MAX; i++) {
        if (curproc->fd_table[i] == NULL) {
            fd = i;
            break;
        }
    }
    // file descriptor table is full
    if (fd == -1) {
        return EMFILE;
    }
    
    struct vnode *vn;

    // open file and put data in vnode
    int fderr = vfs_open(path, tf->tf_a1, tf->tf_a2, &vn);
    if (fderr != 0) {
        return fderr;
    }

    /*  
     * Allocate data from vfs opened file into 
     * open file node (index is same as fd) 
     */
    spinlock_acquire(all_of->of_table_lock);
    all_of->of_nodes[fd]->flags = flags;
    all_of->of_nodes[fd]->fp = 0;
    all_of->of_nodes[fd]->refcount = 0;
    all_of->of_nodes[fd]->vptr = vn;

    
    spinlock_release(all_of->of_table_lock);
    return fd;
}


int sys_open(const char *filename, int flags, mode_t mode) {
    if (of_table == NULL) {
        of_table = kmalloc(sizeof(openf_table));
        of_table->concurrency = lock_create("concurrency");
    }
    lock_acquire(of_table->concurrency);
    
    // get trapframe
    struct trapframe* tf;
    syscall(tf);

    // check filename
    char path[NAME_MAX];
    size_t got;
    int filename_check = copyinstr((const_userptr_t)tf->tf_a0, path, NAME_MAX, &got);
    if (filename_check != 0) {
        lock_release(of_table->concurrency);
        return filename_check;
    }

    // check flags
    int all_flags = O_ACCMODE | O_NOCTTY | O_APPEND | O_TRUNC | O_EXCL | O_CREAT;
    if ((flags & all_flags) != flags) {
        lock_release(of_table->concurrency);
        return EINVAL;
    }
    
    int fd = -1;
    // get free file descripter index in array
    for (int i = 0; i < FD_MAX; i++) {
        if (curproc->fd_table[i] == NULL) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        lock_release(of_table->concurrency);
        return EMFILE;
    }
    
    struct vnode *vn;

    int fderr = vfs_open(path, tf->tf_a1, tf->tf_a2, &vn);

    if (fderr != 0) {
        lock_release(of_table->concurrency);
        return fderr;
    }
    // get free open file index in array
    for (int i = 0; i < OPEN_MAX; i++) {
        if (of_table->of_list[i] == NULL) {
            of_table->of_list[i]->fd = fd;
            of_table->of_list[i]->vptr = vn;
            of_table->of_list[i]->mult_file = lock_create("mult_file");
            // of_table->of_list[i]->refcount = 0;
            of_table->of_list[i]->fp = 0;
            break;
        }
    }
    
    lock_release(of_table->concurrency);
    return fd;
}
