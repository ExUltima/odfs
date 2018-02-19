#include "operation.h"
#include "option.h"

#include <fuse.h>

#include <stddef.h>
#include <stdlib.h>

static struct fuse_operations const ops = {
	.init = operation_init,
	.destroy = operation_destroy
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct options const *opts;
	int res;

	opts = option_init(&args);
	if (!opts) {
		return EXIT_FAILURE;
	}

	if (opts->help) {
		option_print_usage(stdout, args.argv[0]);
		fuse_opt_add_arg(&args, "--help"); // add it back to show fuse help
		args.argv[0] = ""; // tell fuse don't print 'usage: ...' line
	}

	res = fuse_main(args.argc, args.argv, &ops, NULL);
	option_term();

	return res;
}
