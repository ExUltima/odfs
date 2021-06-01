#include "auth.hpp"
#include "option.hpp"

#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <unordered_set>

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

static const std::unordered_set<char> url_reserved = {
	'!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', '/', ':', ';', '=', '?',
	'@', '[', ']'
};

static struct MHD_Daemon *httpd;
static char auth_code[64];

// http utility

static std::string percent_encode(const char *s)
{
	std::stringstream r;

	for (; *s; s++) {
		if (url_reserved.count(*s)) {
			r << '%';
			r << std::setfill('0') << std::setw(2) << std::right << std::hex;
			r << static_cast<int>(*s);
		} else {
			r << *s;
		}
	}

	return r.str();
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

	// setup sign-in url
	std::stringstream url;

	url << "https://login.microsoftonline.com/common/oauth2/v2.0/authorize?";
	url << "client_id=" << opts->app.app_id << '&';
	url << "scope=offline_access%20files.readwrite&";
	url << "response_type=code&";
	url << "redirect_uri=" << percent_encode(opts->app.redirect_uri);

	// redirect
	return redirect(ctx, url.str().c_str());
}

static MHD_Result process_request(
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
	MHD_Result res;

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

bool auth_init(void)
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

void auth_term(void)
{
	MHD_stop_daemon(httpd);
}
