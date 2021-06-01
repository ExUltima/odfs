#include "option.hpp"

#include <fuse_lowlevel.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_APP_ID			"f457aaf7-d90c-406c-95ae-2d590bac9b64"
#define DEFAULT_REDIRECT_URI	"http://localhost:2300/authorization-code"

static struct options opts;

static const struct fuse_opt specs[] = {
	{ "--app-id=%s", offsetof(struct options, app.app_id), 0},
	{ "--redirect-uri=%s", offsetof(struct options, app.redirect_uri), 0},
	{ "--auth-port=%hu", offsetof(struct options, app.auth_port), 0 },
	FUSE_OPT_END
};

static const char * help_text =
"OneDrive specific options:\n"
"    --app-id=GUID          identifier of registered application to use, "
"default is " DEFAULT_APP_ID "\n"
"    --redirect-uri=URI     uri to be redirected to by OneDrive when "
"authentication is completed, default is " DEFAULT_REDIRECT_URI "\n"
"    --auth-port=PORT       port to listen for OAuth authentication, "
"default is 2300\n";

void option_usage(const char *prog)
{
	printf("usage: %s [options] <mountpoint>\n\n", prog);
	printf("%s\n", help_text);
	printf("FUSE specific options:\n");
	fuse_cmdline_help();
	fuse_lowlevel_help();
}

bool option_init(struct fuse_args *args)
{
	// prepare default values
	opts.app.app_id = strdup(DEFAULT_APP_ID);
	if (!opts.app.app_id) {
		fprintf(stderr, "failed to allocate default application id\n");
		return false;
	}

	opts.app.redirect_uri = strdup(DEFAULT_REDIRECT_URI);
	if (!opts.app.redirect_uri) {
		fprintf(stderr, "failed to allocate default redirect uri\n");
		goto fail_with_appid;
	}

	opts.app.auth_port = 2300;

	// parse arguments
	if (fuse_parse_cmdline(args, &opts.fuse) != 0) {
		goto fail_with_redirect_uri;
	}

	if (fuse_opt_parse(args, &opts.app, specs, NULL) == -1) {
		goto fail_with_fuseopts;
	}

	return true;

	// errors handling
	fail_with_fuseopts:
	free(opts.fuse.mountpoint);

	fail_with_redirect_uri:
	free(opts.app.redirect_uri);

	fail_with_appid:
	free(opts.app.app_id);

	return false;
}

void option_term(void)
{
	free(opts.app.app_id);
	free(opts.app.redirect_uri);
	free(opts.fuse.mountpoint);
}

const struct options * option_get(void)
{
	return &opts;
}
