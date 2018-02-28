#ifndef OPERATION_H_INCLUDED
#define OPERATION_H_INCLUDED

#include <fuse_lowlevel.h>

void operation_init(void *userdata, struct fuse_conn_info *conn);
void operation_destroy(void *userdata);

#endif // OPERATION_H_INCLUDED
