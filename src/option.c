#include "option.h"

#include <fuse.h>

#include <stddef.h>
#include <stdio.h>

static struct options opts = {
	.auth_port = 2300
};

static struct fuse_opt const specs[] = {
	{ "--auth-port=%hu", offsetof(struct options, auth_port), 0 },
	{ "-h", offsetof(struct options, help), true },
	{ "--help", offsetof(struct options, help), true },
	FUSE_OPT_END
};

static char const * help_text =
"OneDrive specific options:\n"
"    --auth-port=PORT       port to listen for OAuth authentication, "
"default is 2300\n";

struct options const * option_init(struct fuse_args *args)
{
	if (fuse_opt_parse(args, &opts, specs, NULL) == -1) {
		return NULL;
	}

	return &opts;
}

void option_term(void)
{
}

void option_print_usage(FILE *output, char const *prog)
{
	fprintf(output, "usage: %s [options] <mountpoint>\n\n", prog);
	fprintf(output, "%s\n", help_text);
}

struct options const * option_get(void)
{
	return &opts;
}
