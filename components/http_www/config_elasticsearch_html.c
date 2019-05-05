#include "http.h"

static const char  page[]	asm("_binary_config_elasticsearch_html_start");

static const httpPage_t httpPage = {
	.uri	= "/config_elasticsearch.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageConfigElasticsearchHTMLInit(httpd_handle_t server) {
	httpPageRegister(server, &httpPage);
}