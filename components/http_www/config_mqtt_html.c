#include "http.h"

static const char  page[]	asm("_binary_config_mqtt_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_mqtt.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigMQTTHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}