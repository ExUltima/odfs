#include "option.h"

#include <fuse.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_APP_ID			"f457aaf7-d90c-406c-95ae-2d590bac9b64"
#define DEFAULT_REDIRECT_URI	"http://localhost:2300/authorization-code"

static struct options opts;

static struct fuse_opt const specs[] = {
	{ "--app-id=%s", offsetof(struct options, app_id), 0},
	{ "--redirect-uri=%s", offsetof(struct options, redirect_uri), 0},
	{ "--auth-port=%hu", offsetof(struct options, auth_port), 0 },
	{ "-h", offsetof(struct options, help), true },
	{ "--help", offsetof(struct options, help), true },
	FUSE_OPT_END
};

static char const * help_text =
"OneDrive specific options:\n"
"    --app-id=GUID          identifier of registered application to use, "
"default is " DEFAULT_APP_ID "\n"
"    --redirect-uri=URI     uri to be redirected to by OneDrive when "
"authentication is completed, default is " DEFAULT_REDIRECT_URI "\n"
"    --auth-port=PORT       port to listen for OAuth authentication, "
"default is 2300\n";

struct options const * option_init(struct fuse_args *args)
{
	// prepare default values
	opts.app_id = strdup(DEFAULT_APP_ID);
	if (!opts.app_id) {
		goto fail;
	}

	opts.redirect_uri = strdup(DEFAULT_REDIRECT_URI);
	if (!opts.redirect_uri) {
		goto fail;
	}

	opts.auth_port = 2300;

	// parse arguments
	if (fuse_opt_parse(args, &opts, specs, NULL) == -1) {
		goto fail;
	}

	return &opts;

	// error handling
	fail:
	option_term();

	return NULL;
}

void option_term(void)
{
	free(opts.app_id);
	free(opts.redirect_uri);
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
