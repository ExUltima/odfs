#ifndef OPTION_HPP
#define OPTION_HPP

#include <fuse_lowlevel.h>

#include <stdbool.h>

struct options {
	struct fuse_cmdline_opts fuse;
	struct {
		char *app_id;
		char *redirect_uri;
		unsigned short auth_port;
	} app;
};

void option_usage(const char *prog);

bool option_init(struct fuse_args *args);
void option_term(void);

const struct options * option_get(void);

#endif // OPTION_HPP
