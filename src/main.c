#include "operation.h"
#include "option.h"

#include <fuse_lowlevel.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static const struct fuse_lowlevel_ops ops = {
	.init = operation_init,
	.destroy = operation_destroy
};

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	int res = EXIT_SUCCESS;
	const struct options *opt;
	struct fuse_session *fuse;

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
	}

	// initialize fuse
	fuse = fuse_session_new(&args, &ops, sizeof(ops), NULL);
	if (!fuse) {
		res = EXIT_FAILURE;
		goto cleanup_with_opt;
	}

	if (fuse_set_signal_handlers(fuse) != 0) {
		res = EXIT_FAILURE;
		goto cleanup_with_fuse;
	}

	if (fuse_session_mount(fuse, opt->fuse.mountpoint) != 0) {
		res = EXIT_FAILURE;
		goto cleanup_with_signal;
	}

	// enter main loop
	if (fuse_daemonize(opt->fuse.foreground) != 0) {
		res = EXIT_FAILURE;
		goto cleanup_with_mount;
	}

	if (opt->fuse.singlethread) {
		res = fuse_session_loop(fuse);
	} else {
		res = fuse_session_loop_mt(fuse, opt->fuse.clone_fd);
	}

	// clean up
	cleanup_with_mount:
	fuse_session_unmount(fuse);

	cleanup_with_signal:
	fuse_remove_signal_handlers(fuse);

	cleanup_with_fuse:
	fuse_session_destroy(fuse);

	cleanup_with_opt:
	option_term();

	cleanup:
	fuse_opt_free_args(&args);

	return res;
}
