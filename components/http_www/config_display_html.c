#include "http.h"

static const char  page[]	asm("_binary_config_display_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_display.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigDisplayHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}