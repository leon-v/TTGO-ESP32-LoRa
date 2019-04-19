#include "http.h"

static const char  pageStart[]	asm("_binary_config_diesensors_html_start");
static const char  pageEnd[]	asm("_binary_config_diesensors_html_end");

static const ssiTag_t ssiTags[] = {

	{"dieSensEn>>0",		SSI_TYPE_CHECKBOX},
	{"dieSensEn>>1",		SSI_TYPE_CHECKBOX},
	{"dieSensEn>>2",		SSI_TYPE_CHECKBOX},

	{"delayTemp",			SSI_TYPE_INTEGER},
	{"delayHall",			SSI_TYPE_INTEGER},
	{"delayBattV",			SSI_TYPE_INTEGER},


	{"dieSensInRt>>0",		SSI_TYPE_CHECKBOX},
	{"dieSensInRt>>1",		SSI_TYPE_CHECKBOX},
	{"dieSensInRt>>2",		SSI_TYPE_CHECKBOX},
	{"dieSensInRt>>3",		SSI_TYPE_CHECKBOX},
};

static esp_err_t handler(httpd_req_t *req) {
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	return httpRespond(req, pageStart, pageEnd, (ssiTag_t *) &ssiTags, sizeof(ssiTags) / sizeof(ssiTags[0]));
}

static httpd_uri_t getURI = {
    .uri      = "/config_diesensors.html",
    .method   = HTTP_GET,
    .handler  = handler
};

static httpd_uri_t postURI = {
    .uri      = "/config_diesensors.html",
    .method   = HTTP_POST,
    .handler  = handler
};



void httpPageConfigDieSensorsHTMLInit(httpd_handle_t server) {
	httpd_register_uri_handler(server, &getURI);
	httpd_register_uri_handler(server, &postURI);
}
