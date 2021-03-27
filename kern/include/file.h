/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>
#include <vnode.h>
#include <spinlock.h>


/*
 * Put your function declarations and data types here ...
 */

int sys_open(const char *filename, int flags, mode_t mode, int *retval);
int sys_close (int fd);
int sys_read(int fd, void *buf, size_t count, ssize_t *retval);
int sys_write (int fd, const void *buf, size_t count, ssize_t *retval);
int sys_lseek(int fd, off_t offset, int whence, off_t *retval64);

typedef struct open_file_node {
    int flags;
    off_t fp;                   /* Offset */
    int refcount;               /* For dup2 and fork */
    struct vnode *vn;
} of_node;

struct lock *of_table_lock;
of_node *of_table[OPEN_MAX];

#endif /* _FILE_H_ */
