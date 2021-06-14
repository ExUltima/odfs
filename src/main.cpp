#include "option.hpp"
#include "program.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <ostream>

#include <fuse_common.h>
#include <fuse_lowlevel.h>
#include <fuse_opt.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

static bool daemonize(struct fuse_args *args)
{
	int pipes[2];
	bool res;

	if (pipe(pipes) < 0) {
		perror("failed to create parent/child communication channel");
		return false;
	}

	std::unique_ptr<program> prog;

	switch (fork()) {
	case -1:
		// failed
		perror("failed to spawn background process");
		close(pipes[0]);
		close(pipes[1]);
		res = false;
		break;
	case 0:
		// we are in the child
		close(pipes[0]);

		try {
			prog.reset(new program(args));
			prog->init_forked();
			res = true;
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			res = false;
		}

		if (write(pipes[1], &res, sizeof(res)) < 0) {
			perror("failed to write background initialization status");
		}

		close(pipes[1]);

		if (res) {
			try {
				prog->run();
			} catch (std::exception &e) {
				std::cerr << e.what() << std::endl;
				res = false;
			}
		}
		break;
	default:
		// we are in the parent
		close(pipes[1]);

		switch (read(pipes[0], &res, sizeof(res))) {
		case -1:
			perror("failed to read background initialization status");
			res = false;
			break;
		case sizeof(res):
			break;
		default:
			fprintf(stderr, "got incompleted background initialization status");
			res = false;
		}

		close(pipes[0]);
	}

	return res;
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	int res = EXIT_SUCCESS;
	const struct options *opt;

	// parse arguments
	if (!option_init(&args)) {
		res = EXIT_FAILURE;
		goto cleanup;
	}

	opt = option_get();

	if (opt->fuse.show_help) {
		option_usage(argv[0]);
		goto cleanup_with_opt;
	} else if (opt->fuse.show_version) {
		printf("FUSE library version %s\n", fuse_pkgversion());
		fuse_lowlevel_version();
		goto cleanup_with_opt;
	} else if (!opt->fuse.mountpoint) {
		fprintf(stderr, "no mount point specified\n");
		res = EXIT_FAILURE;
		goto cleanup_with_opt;
	}

	// spawn background process and wait until it enter main loop
	if (opt->fuse.foreground) {
		try {
			program p(&args);

			p.init();
			p.run();
		} catch (std::exception &e) {
			std::cerr << e.what() << std::endl;
			res = EXIT_FAILURE;
		}
	} else {
		res = daemonize(&args) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// clean up
cleanup_with_opt:
	option_term();

cleanup:
	fuse_opt_free_args(&args);

	return res;
}
