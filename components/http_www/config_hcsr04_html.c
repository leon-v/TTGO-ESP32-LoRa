#include "http.h"

static const char  page[]	asm("_binary_config_hcsr04_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_hcsr04.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigHCSR04HTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}