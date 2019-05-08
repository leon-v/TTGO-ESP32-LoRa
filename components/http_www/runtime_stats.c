#include "http.h"

static const char  page[]	asm("_binary_runtime_stats_html_start");

static const httpPage_t httpPage = {
	.uri	= "/runtime_stats.html",
	.page	= page,
	.type	= HTTPD_TYPE_TEXT
};

void httpPageRuntimeStatsInit(httpd_handle_t server){
	httpPageRegister(server, &httpPage);
}