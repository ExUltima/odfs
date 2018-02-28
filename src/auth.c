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

struct reqctx {
	struct MHD_Connection *conn;
	unsigned status;
};

#define AUTHORIZE_URL \
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
static char auth_code[64];

// http utility

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

// response utility

static struct MHD_Response * ok(struct reqctx *ctx, const char *msg)
{
	ctx->status = MHD_HTTP_OK;

	return MHD_create_response_from_buffer(
		strlen(msg),
		(void *) msg,
		MHD_RESPMEM_MUST_COPY
	);
}

static struct MHD_Response * bad_request(struct reqctx *ctx, const char *reason)
{
	ctx->status = MHD_HTTP_BAD_REQUEST;

	return MHD_create_response_from_buffer(
		strlen(reason),
		(void *) reason,
		MHD_RESPMEM_MUST_COPY
	);
}

static struct MHD_Response * internal_server_error(
	struct reqctx *ctx,
	const char *reason)
{
	ctx->status = MHD_HTTP_INTERNAL_SERVER_ERROR;

	return MHD_create_response_from_buffer(
		strlen(reason),
		(void *) reason,
		MHD_RESPMEM_MUST_COPY
	);
}

static struct MHD_Response * redirect(struct reqctx *ctx, const char *location)
{
	struct MHD_Response *resp;

	resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
	if (!resp) {
		return NULL;
	}

	if (MHD_add_response_header(resp, "Location", location) == MHD_NO) {
		MHD_destroy_response(resp);
		return NULL;
	}

	ctx->status = MHD_HTTP_SEE_OTHER;
	return resp;
}

// response handlers

static struct MHD_Response * process_authorization_code(struct reqctx *ctx)
{
	const char *code = MHD_lookup_connection_value(
		ctx->conn,
		MHD_GET_ARGUMENT_KIND,
		"code"
	);

	if (!code) {
		return bad_request(
			ctx,
			"<p>"
			"No authorization code in the URL that was redirected by OneDrive."
			"</p>"
		);
	} else if (strlen(code) >= sizeof(auth_code)) {
		return bad_request(ctx, "<p>The authorization code is too long.</p>");
	}

	strcpy(auth_code, code);

	return ok(
		ctx,
		"<p>Authorization is completed. You can close the browser now.</p>"
	);
}

static struct MHD_Response * process_get_login(struct reqctx *ctx)
{
	const struct options *opts = option_get();
	char *redirect_uri, *url;
	int res;
	struct MHD_Response *resp;

	// setup sign-in url
	redirect_uri = percent_encode(opts->app.redirect_uri);
	if (!redirect_uri) {
		return internal_server_error(
			ctx,
			"<p>insufficient memory for encoding redirect uri</p>"
		);
	}

	res = snprintf(NULL, 0, AUTHORIZE_URL, opts->app.app_id, redirect_uri) + 1;
	url = malloc(res);

	if (!url) {
		free(redirect_uri);
		return internal_server_error(
			ctx,
			"insufficient memory for login url\n"
		);
	}

	snprintf(url, res, AUTHORIZE_URL, opts->app.app_id, redirect_uri);
	free(redirect_uri);

	// redirect
	resp = redirect(ctx, url);
	free(url);

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
	struct reqctx ctx;
	struct MHD_Response *resp;
	int res;

	// process request
	memset(&ctx, 0, sizeof(ctx));
	ctx.conn = connection;
	ctx.status = MHD_HTTP_OK;

	if (strcmp(url, "/") == 0) {
		resp = process_get_login(&ctx);
	} else if (strcmp(url, "/authorization-code") == 0) {
		resp = process_authorization_code(&ctx);
	} else {
		ctx.status = MHD_HTTP_NOT_FOUND;
		resp = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
	}

	if (!resp) {
		fprintf(stderr, "failed to create response\n");
		return MHD_NO;
	}

	// response
	res = MHD_queue_response(connection, ctx.status, resp);
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
	addr.sin_port = htons(option_get()->app.auth_port);

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
