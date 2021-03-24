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
int sys_open(const char *filename, int flags) {
    
    syscall()
    return vfs_open(path, flags, mode, vnode);
    
}

int sys_open(const char *filename, int flags, mode_t mode) {
    vfs_open(filename, flags, mode);

}
