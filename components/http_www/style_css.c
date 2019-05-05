#include "http.h"

extern const char  page[]	asm("_binary_style_css_start");

static const httpPage_t httpPage = {
	.uri	= "/style.css",
	.page	= page,
	.type	= "text/css"
};

void httpPageStyleCSSInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}