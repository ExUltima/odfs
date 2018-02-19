#include "auth.h"
#include "option.h"

#include <microhttpd.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

static struct MHD_Daemon *httpd;

static int process_request(
	void *cls,
	struct MHD_Connection *connection,
	const char *url,
	const char *method,
	const char *version,
	const char *upload_data,
	size_t *upload_data_size,
	void **con_cls)
{
	struct MHD_Response *resp;
	int res;
	const char *page  = "<h1>Hello, browser!</h1>";

	resp = MHD_create_response_from_buffer(
		strlen(page),
		(void *)page,
		MHD_RESPMEM_PERSISTENT);

	if (!resp) {
		return MHD_NO;
	}

	res = MHD_queue_response(connection, MHD_HTTP_OK, resp);
	MHD_destroy_response(resp);

	return res;
}

static void cleanup_request(
	void *cls,
	struct MHD_Connection *connection,
	void **con_cls,
	enum MHD_RequestTerminationCode toe)
{
}

bool auth_start_listen(void)
{
	struct sockaddr_in addr = { 0 };

	// setup server address
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(option_get()->auth_port);

	// start server
	httpd = MHD_start_daemon(
		MHD_USE_SELECT_INTERNALLY | MHD_USE_POLL,
		0,
		NULL,
		NULL,
		process_request,
		NULL,
		MHD_OPTION_NOTIFY_COMPLETED,
		cleanup_request,
		NULL,
		MHD_OPTION_SOCK_ADDR,
		&addr,
		MHD_OPTION_END);

	if (!httpd) {
		fprintf(stderr, "failed to start authentication server\n");
		return false;
	}

	return true;
}

void auth_stop_listen(void)
{
	MHD_stop_daemon(httpd);
}
