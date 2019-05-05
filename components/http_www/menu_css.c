#include "http.h"

static const char  page[]	asm("_binary_menu_css_start");

static const httpPage_t httpPage = {
	.uri	= "/menu.css",
	.page	= page,
	.type	= "text/css"
};

void httpPageMenuCSSInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}