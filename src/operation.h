#ifndef OPERATION_H_INCLUDED
#define OPERATION_H_INCLUDED

#include <fuse.h>

void * operation_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
void operation_destroy(void *private_data);

#endif // OPERATION_H_INCLUDED
