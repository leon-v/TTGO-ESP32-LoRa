#include "http.h"

static const char  page[]	asm("_binary_config_device_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_device.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigDeviceHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}
