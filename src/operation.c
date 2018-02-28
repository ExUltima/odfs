#include "operation.h"
#include "auth.h"

#include <stdlib.h>

void operation_init(void *userdata, struct fuse_conn_info *conn)
{
	if (!auth_start_listen()) {
		exit(EXIT_FAILURE);
	}
}

void operation_destroy(void *userdata)
{
	auth_stop_listen();
}
