#include "http.h"


extern const char  page[]	asm("_binary_index_html_start");

static const httpPage_t httpPage = {
	.uri	= "/",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageIndexHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}