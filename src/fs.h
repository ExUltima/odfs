#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <fuse_lowlevel.h>

void fs_init(void *userdata, struct fuse_conn_info *conn);
void fs_destroy(void *userdata);

#endif // FS_H_INCLUDED
