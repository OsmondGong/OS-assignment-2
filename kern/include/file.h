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
    //int fd;
    int flags;
    int fp;                   /* Offset */
    int refcount;               /* For dup2 and fork */
    struct vnode *vptr;
    //struct lock *mult_file;     
} of_node;


typedef struct open_file_table {
    struct spinlock *of_table_spinlock;
    of_node *of_nodes[OPEN_MAX];
} of_table;

#endif /* _FILE_H_ */
