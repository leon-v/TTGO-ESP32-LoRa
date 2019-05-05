#include "http.h"

static const char  page[]	asm("_binary_config_lora_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_lora.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigLoRaHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}