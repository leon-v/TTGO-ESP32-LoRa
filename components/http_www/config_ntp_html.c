#include "http.h"

static const char  page[]	asm("_binary_config_ntp_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_ntp.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigNTPHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}