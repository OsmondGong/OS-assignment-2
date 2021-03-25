/*
 * Declarations for file handle and file table management.
 */

#ifndef _FILE_H_
#define _FILE_H_

/*
 * Contains some file-related maximum length constants
 */
#include <limits.h>


/*
 * Put your function declarations and data types here ...
 */

typedef struct open_file_node {
    int fd;
    struct lock *mult_file;
    off_t fp;
    // int refcount;
    vnode *vptr;
} of_node;


typedef struct open_file_table {
    struct lock *concurrency;
    of_node of_list[OPEN_MAX];
} openf_table

#endif /* _FILE_H_ */
