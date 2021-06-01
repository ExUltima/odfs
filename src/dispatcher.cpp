#include "dispatcher.hpp"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <sys/time.h>

struct fd_data {
	dispatcher_handler_t rh;
	dispatcher_handler_t wh;
};

static fd_set fds;
static struct fd_data data[FD_SETSIZE];
static int lastfd = -1;

bool dispatcher_init(void)
{
	FD_ZERO(&fds);

	return true;
}

void dispatcher_term(void)
{
}

void dispatcher_add(int fd, dispatcher_handler_t rh, dispatcher_handler_t wh)
{
	// sanity checks
	assert(fd >= 0);
	assert(rh);
	assert(wh);

	assert(FD_ISSET(fd, &fds) == 0);
	assert(data[fd].rh == NULL);
	assert(data[fd].wh == NULL);

	if (fd >= FD_SETSIZE) {
		fprintf(stderr, "opened too much file descriptors\n");
		abort();
	}

	// add fd
	FD_SET(fd, &fds);

	data[fd].rh = rh;
	data[fd].wh = wh;

	if (fd > lastfd) {
		lastfd = fd;
	}
}

bool dispatcher_remove(int fd)
{
	assert(fd >= 0);

	if (!FD_ISSET(fd, &fds)) {
		fprintf(stderr, "trying to remove invalid file descriptor %d\n", fd);
		return false;
	}

	FD_CLR(fd, &fds);
	memset(&data[fd], 0, sizeof(data[fd]));

	if (fd == lastfd) {
		for (lastfd--; lastfd >= 0; lastfd--) {
			if (FD_ISSET(lastfd, &fds)) {
				break;
			}
		}
	}
}

int dispatcher_dispatch(void)
{
	fd_set readable, writable;
	int n;

	memcpy(&readable, &fds, sizeof(fd_set));
	memcpy(&writable, &fds, sizeof(fd_set));

	// wait for events
	n = select(lastfd + 1, &readable, &writable, NULL, NULL);

	if (n < 0) {
		int e = errno;

		if (e == EINTR) {
			return 0;
		} else {
			perror("failed to wait for events");
			return -e;
		}
	}

	// process events
	for (int fd = 0; n > 0; fd++) {
		int r;

		if (FD_ISSET(fd, &readable)) {
			r = data[fd].rh(fd);
			if (r != 0) {
				return r;
			}
			n--;
		}

		if (FD_ISSET(fd, &writable)) {
			r = data[fd].wh(fd);
			if (r != 0) {
				return r;
			}
			n--;
		}
	}

	return 0;
}
