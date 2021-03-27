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

typedef struct open_file_node {
    int flags;
    int fp;                   /* Offset */
    int refcount;               /* For dup2 and fork */
    struct vnode *vn;
} of_node;


#endif /* _FILE_H_ */
