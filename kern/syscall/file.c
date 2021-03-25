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
of_table all_fp[OPEN_MAX];


int sys_open(const char *filename, int flags) {
    // check filename
    char path[NAME_MAX];
    size_t got;
    int filename_check = copyinstr((const_userptr_t)filename, path, NAME_MAX, &got);
    if (filename_check != 0) {
        return filename_check;
    }
    // check flags
    if (flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR) {
        return -1;
    }
    

    struct proc *cur = curproc;
    for (int i = 0; i < FD_MAX; i++)
        if (cur->fd_table[i] == NULL) {
            break;
        }


    open = vfs_open(path, flags, mode, vnode);
}


int sys_open(const char *filename, int flags, mode_t mode) {
    //vfs_open(filename, flags, mode);
    return 0;
}
