#include "http.h"

static const char  page[]	asm("_binary_config_wifi_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_wifi.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigWiFiHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}