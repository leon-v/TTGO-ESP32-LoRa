#include "http.h"

extern const char  page[]	asm("_binary_javascript_js_start");

static const httpPage_t httpPage = {
	.uri	= "/javascript.js",
	.page	= page,
	.type	= "application/javascript"
};

void httpPageJavascriptJSInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}