#ifndef OPTION_H_INCLUDED
#define OPTION_H_INCLUDED

#include <fuse.h>

#include <stdbool.h>

struct options {
	bool help;
};

struct options const * option_init(struct fuse_args *args);

#endif // OPTION_H_INCLUDED
