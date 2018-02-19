#include "operation.h"
#include "auth.h"

#include <stddef.h>
#include <stdlib.h>

void * operation_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
	if (!auth_start_listen()) {
		exit(EXIT_FAILURE);
	}

	return NULL;
}

void operation_destroy(void *private_data)
{
	auth_stop_listen();
}
