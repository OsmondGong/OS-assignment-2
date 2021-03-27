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

typedef struct open_file_node {
    int flags;
    int fp;                   /* Offset */
    int refcount;               /* For dup2 and fork */
    struct vnode *vn;
} of_node;


#endif /* _FILE_H_ */
