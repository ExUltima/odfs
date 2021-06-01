#ifndef FS_HPP
#define FS_HPP

#include <fuse_lowlevel.h>

void fs_init(void *userdata, struct fuse_conn_info *conn);
void fs_destroy(void *userdata);

#endif // FS_HPP
