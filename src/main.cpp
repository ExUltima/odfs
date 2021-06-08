#include "auth.hpp"
#include "client.hpp"
#include "fs.hpp"
#include "io.hpp"
#include "option.hpp"

#include <boost/asio/error.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <ostream>

#include <curl/curl.h>

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

static fuse_session *session;
static boost::asio::signal_set signals(asio, SIGINT, SIGTERM);
static boost::asio::posix::stream_descriptor fuse(asio);

static const struct fuse_lowlevel_ops ops = {
	.init = fs_init,
	.destroy = fs_destroy
};

static void handle_signal(boost::system::error_code const &ec, int sig)
{
	if (ec) {
		if (ec.value() != boost::asio::error::operation_aborted) {
			std::cerr << ec.message() << std::endl;
		}
	} else {
		fuse_session_exit(session);
	}
}

static void handle_fuse_read(boost::system::error_code const &ec)
{
	if (ec) {
		if (ec.value() != boost::asio::error::operation_aborted) {
			std::cerr << ec.message() << std::endl;
			fuse_session_exit(session);
		}
		return;
	}

	// read message
	struct fuse_buf buf;
	int r;

	memset(&buf, 0, sizeof(buf));

	r = fuse_session_receive_buf(session, &buf);

	if (r <= 0) {
		if (r < 0) {
			const char *m = strerror(abs(r));
			std::cerr << "failed to read FUSE message: " << m << std::endl;
		} else {
			fuse_session_exit(session);
		}
	}

	// process message
	fuse_session_process_buf(session, &buf);
	free(buf.mem);

	fuse.async_wait(
		boost::asio::posix::descriptor_base::wait_read,
		handle_fuse_read);
}

static bool init_fuse(struct fuse_args *args)
{
	session = fuse_session_new(args, &ops, sizeof(ops), NULL);
	if (!session) {
		fprintf(stderr, "failed to initialize FUSE\n");
		return false;
	}

	if (fuse_session_mount(session, option_get()->fuse.mountpoint) < 0) {
		fprintf(stderr, "mount failed\n");
		goto fail_with_fuse;
	}

	try {
		fuse.assign(fuse_session_fd(session));
		fuse.async_wait(
			boost::asio::posix::descriptor_base::wait_read,
			handle_fuse_read);
	} catch (...) {
		fuse.release();
		goto fail_with_fuse;
	}

	return true;

fail_with_fuse:
	fuse_session_destroy(session);
	session = NULL;

	return false;
}

static void term_fuse(void)
{
	try {
		fuse.release();
	} catch (...) {
	}

	fuse_session_unmount(session);
	fuse_session_destroy(session);

	session = NULL;
}

static bool init(struct fuse_args *args)
{
	// external libraries
	if (curl_global_init(CURL_GLOBAL_ALL | CURL_GLOBAL_ACK_EINTR) != 0) {
		fprintf(stderr, "failed to initialize CURL\n");
		return false;
	}

	// internal modules
	if (!client_init()) {
		goto fail_with_curl;
	}

	if (!auth_init()) {
		goto fail_with_client;
	}

	if (!init_fuse(args)) {
		goto fail_with_auth;
	}

	return true;

fail_with_auth:
	auth_term();

fail_with_client:
	client_term();

fail_with_curl:
	curl_global_cleanup();

	return false;
}

static bool run(void)
{
	try {
		signals.async_wait(handle_signal);
	} catch (...) {
		return false;
	}

	while (!fuse_session_exited(session)) {
		try {
			if (!asio.run_one()) {
				signals.cancel();
				return false;
			}
		} catch (...) {
			signals.cancel();
			return false;
		}
	}

	signals.cancel();

	return true;
}

static void term(void)
{
	// internal modules
	term_fuse();
	auth_term();
	client_term();

	// external libraries
	curl_global_cleanup();
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

	try {
		asio.notify_fork(boost::asio::io_context::fork_prepare);
	} catch (...) {
		close(pipes[0]);
		close(pipes[1]);
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

		try {
			asio.notify_fork(boost::asio::io_context::fork_child);
			res = init_child(args);
		} catch (...) {
			res = false;
		}

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

		try {
			asio.notify_fork(boost::asio::io_context::fork_parent);

			switch (read(pipes[0], &res, sizeof(res))) {
			case -1:
				perror("failed to read background initialization status");
				res = false;
				break;
			case sizeof(res):
				break;
			default:
				fprintf(
					stderr,
					"got incompleted background initialization status");
				res = false;
			}
		} catch (...) {
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
