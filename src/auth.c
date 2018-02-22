#include "auth.h"
#include "option.h"

#include <microhttpd.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#define LOGIN_URL \
	"https://login.microsoftonline.com/common/oauth2/v2.0/authorize?" \
	"client_id=%s&" \
	"scope=offline_access%%20files.readwrite&" \
	"response_type=code&" \
	"redirect_uri=%s"

#define RESERVED(ch) [ch] = true

static const bool url_reserved[CHAR_MAX] = {
	RESERVED('!'), RESERVED('#'), RESERVED('$'), RESERVED('&'), RESERVED('\''),
	RESERVED('('), RESERVED(')'), RESERVED('*'), RESERVED('+'), RESERVED(','),
	RESERVED('/'), RESERVED(':'), RESERVED(';'), RESERVED('='), RESERVED('?'),
	RESERVED('@'), RESERVED('['), RESERVED(']')
};

static struct MHD_Daemon *httpd;

static char * percent_encode(const char *s)
{
	size_t len, i;
	char *buf;

	// prepare buffer
	len = strlen(s);
	buf = malloc(len * 3 + 1); // we need 3 character to represent 1 character

	if (!buf) {
		return NULL;
	}

	// encode
	for (i = 0; *s; s++) {
		if (!url_reserved[*s]) {
			buf[i++] = *s;
		} else {
			sprintf(&buf[i], "%%%02x", *s);
			i += 3;
		}
	}

	buf[i] = 0;
	return buf;
}

static struct MHD_Response * process_get_login(unsigned int *status)
{
	const struct options *opts = option_get();
	char *redirect_uri, *url;
	int res;
	struct MHD_Response *resp;

	// setup sign-in url
	redirect_uri = percent_encode(opts->redirect_uri);
	if (!redirect_uri) {
		fprintf(stderr, "insufficient memory for encoding redirect uri\n");
		return NULL;
	}

	res = snprintf(NULL, 0, LOGIN_URL, opts->app_id, redirect_uri) + 1;
	url = malloc(res);

	if (!url) {
		fprintf(stderr, "insufficient memory for login url\n");
		free(redirect_uri);
		return NULL;
	}

	snprintf(url, res, LOGIN_URL, opts->app_id, redirect_uri);
	free(redirect_uri);

	// setup response
	resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
	if (!resp) {
		fprintf(
			stderr,
			"failed to create response for redirect to login page\n");
		free(url);
		return NULL;
	}

	res = MHD_add_response_header(resp, "Location", url);
	free(url);

	if (res == MHD_NO) {
		fprintf(stderr, "failed to set redirect location\n");
		MHD_destroy_response(resp);
		return NULL;
	}

	*status = MHD_HTTP_SEE_OTHER;

	return resp;
}

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
	unsigned int status;
	int res;

	// process request
	status = MHD_HTTP_OK;

	if (strcmp(url, "/") == 0) {
		resp = process_get_login(&status);
	} else {
		status = MHD_HTTP_NOT_FOUND;
		resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
	}

	if (!resp) {
		return MHD_NO;
	}

	// response
	res = MHD_queue_response(connection, status, resp);
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
