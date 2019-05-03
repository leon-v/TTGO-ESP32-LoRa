#include "http.h"

static const char  page[]	asm("_binary_config_diesensors_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_diesensors.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigDieSensorsHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}