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
struct open_file_table {
    int fp;
    vnode *vptr;
}


#endif /* _FILE_H_ */
