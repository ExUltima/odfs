#include "client.h"

#include <curl/curl.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static CURLM *curl;

bool client_init(void)
{
	curl_version_info_data *info;
	bool http, https;

	// check if curl have required features
	info = curl_version_info(CURLVERSION_NOW);

	if (!(info->features & CURL_VERSION_SSL)) {
		fprintf(stderr, "SSL is not supported by CURL\n");
		return false;
	}

	http = false;
	https = false;

	for (const char *const *proto = info->protocols; *proto != NULL; proto++) {
		if (strcmp(*proto, "http") == 0) {
			http = true;
		} else if (strcmp(*proto, "https") == 0) {
			https = true;
		}
	}

	if (!http) {
		fprintf(stderr, "HTTP is not supported by CURL\n");
		return false;
	}

	if (!https) {
		fprintf(stderr, "HTTPS is not supported by CURL\n");
		return false;
	}

	// init curl
	curl = curl_multi_init();
	if (!curl) {
		fprintf(stderr, "failed to create CURL multi-handle\n");
		return false;
	}

	return true;
}

void client_term(void)
{
	if (curl_multi_cleanup(curl) != CURLM_OK) {
		fprintf(stderr, "failed to destroy CURL multi-handle\n");
	}
}
