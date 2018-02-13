#include "option.h"

#include <fuse.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static struct fuse_operations const ops = {
};

static void print_help(FILE *output, char const *prog)
{
	fprintf(output, "usage: %s [options] <mountpoint>\n\n", prog);
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct options const *opts;

	// parse arguments
	opts = option_init(&args);
	if (!opts) {
		return EXIT_FAILURE;
	}

	if (opts->help) {
		print_help(stdout, args.argv[0]);
		fuse_opt_add_arg(&args, "--help"); // add back to show fuse help
		args.argv[0] = ""; // tell fuse don't print 'usage: ...' line
	}

	// enter main loop
	return fuse_main(args.argc, args.argv, &ops, NULL);
}
