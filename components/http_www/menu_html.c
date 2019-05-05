#include "http.h"

static const char  page[]	asm("_binary_menu_html_start");

static const httpPage_t httpPage = {
	.uri	= "/menu.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageMenuHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}