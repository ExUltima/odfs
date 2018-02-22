#ifndef OPTION_H_INCLUDED
#define OPTION_H_INCLUDED

#include <fuse.h>

#include <stdbool.h>
#include <stdio.h>

struct options {
	char *app_id;
	char *redirect_uri;
	unsigned short auth_port;
	bool help;
};

struct options const * option_init(struct fuse_args *args);
void option_term(void);

void option_print_usage(FILE *output, char const *prog);
struct options const * option_get(void);

#endif // OPTION_H_INCLUDED
