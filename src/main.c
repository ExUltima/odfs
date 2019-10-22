#include "auth.h"
#include "dispatcher.h"
#include "operation.h"
#include "option.h"

#include <fuse_common.h>
#include <fuse_lowlevel.h>
#include <fuse_opt.h>

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

static struct fuse_session *fuse;

static const struct fuse_lowlevel_ops ops = {
	.init = operation_init,
	.destroy = operation_destroy
};

static void handle_signal(int sig)
{
	fuse_session_exit(fuse);
}

static int handle_fuse_read(int fd)
{
	struct fuse_buf buf;
	int r;

	// read message
	memset(&buf, 0, sizeof(buf));

	r = fuse_session_receive_buf(fuse, &buf);

	if (r <= 0) {
		if (r < 0) {
			const char *m = strerror(abs(r));
			fprintf(stderr, "failed to read FUSE message: %s\n", m);
		} else {
			fuse_session_exit(fuse);
		}
		return r;
	}

	// process message
	fuse_session_process_buf(fuse, &buf);
	free(buf.mem);

	return 0;
}

static int handle_fuse_write(int fd)
{
	return 0;
}

static bool init_fuse(struct fuse_args *args)
{
	fuse = fuse_session_new(args, &ops, sizeof(ops), NULL);
	if (!fuse) {
		fprintf(stderr, "failed to initialize FUSE\n");
		return false;
	}

	if (fuse_session_mount(fuse, option_get()->fuse.mountpoint) < 0) {
		fprintf(stderr, "mount failed\n");
		goto fail_with_fuse;
	}

	dispatcher_add(fuse_session_fd(fuse), handle_fuse_read, handle_fuse_write);

	return true;

fail_with_fuse:
	fuse_session_destroy(fuse);
	fuse = NULL;

	return false;
}

static void term_fuse(void)
{
	dispatcher_remove(fuse_session_fd(fuse));

	fuse_session_unmount(fuse);
	fuse_session_destroy(fuse);

	fuse = NULL;
}

static bool init_signal(void)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));

	act.sa_handler = handle_signal;

	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to setup handler for SIGINT");
		return false;
	}

	if (sigaction(SIGTERM, &act, NULL) < 0) {
		perror("failed to setup handler for SIGTERM");
		goto fail_with_int;
	}

	return true;

fail_with_int:
	act.sa_handler = SIG_DFL;

	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to restore handler for SIGINT");
	}

	return false;
}

static void term_signal(void)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));

	act.sa_handler = SIG_DFL;

	if (sigaction(SIGTERM, &act, NULL) < 0) {
		perror("failed to restore handler for SIGTERM");
	}

	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("failed to store handler for SIGINT");
	}
}

static bool init(struct fuse_args *args)
{
	if (!init_signal()) {
		return false;
	}

	if (!dispatcher_init()) {
		goto fail_with_signal;
	}

	if (!auth_init()) {
		goto fail_with_dispatcher;
	}

	if (!init_fuse(args)) {
		goto fail_with_auth;
	}

	return true;

fail_with_auth:
	auth_term();

fail_with_dispatcher:
	dispatcher_term();

fail_with_signal:
	term_signal();

	return false;
}

static bool run(void)
{
	int r;

	while (!fuse_session_exited(fuse)) {
		r = dispatcher_dispatch();
		if (r != 0) {
			return false;
		}
	}

	return true;
}

static void term(void)
{
	term_fuse();
	auth_term();
	dispatcher_term();
	term_signal();
}

static bool init_child(struct fuse_args *args)
{
	if (setsid() < 0) {
		perror("failed to start a new session");
		return false;
	}

	if (chdir("/") < 0) {
		perror("failed to change directory to /");
		return false;
	}

	return init(args);
}

static bool daemonize(struct fuse_args *args)
{
	int pipes[2];
	bool res;

	if (pipe(pipes) < 0) {
		perror("failed to create parent/child communication channel");
		return false;
	}

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

		res = init_child(args);

		if (write(pipes[1], &res, sizeof(res)) < 0) {
			perror("failed to write background initialization status");
		}

		close(pipes[1]);

		if (res) {
			res = run();
			term();
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
		if (!init(&args)) {
			res = EXIT_FAILURE;
		} else {
			res = run() ? EXIT_SUCCESS : EXIT_FAILURE;
			term();
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
