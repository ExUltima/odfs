#include "option.h"

#include <fuse.h>

#include <stddef.h>

static struct options opts;

static struct fuse_opt const specs[] = {
	{ "-h", offsetof(struct options, help), true},
	{ "--help", offsetof(struct options, help), true },
	FUSE_OPT_END
};

struct options const * option_init(struct fuse_args *args)
{
	if (fuse_opt_parse(args, &opts, specs, NULL) == -1) {
		return NULL;
	}

	return &opts;
}
