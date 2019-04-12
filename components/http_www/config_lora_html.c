#include "http.h"

static const char  pageStart[]	asm("_binary_config_lora_html_start");
static const char  pageEnd[]	asm("_binary_config_lora_html_end");

static const ssiTag_t ssiTags[] = {
	{"loraTXSyncWord",	SSI_TYPE_INTEGER},
	{"loraRXSyncWord",	SSI_TYPE_INTEGER},
	{"loraFrequency",	SSI_TYPE_INTEGER},
	{"loraKey",			SSI_TYPE_TEXT},
	{"loraToMQTT",		SSI_TYPE_CHECKBOX},
	{"loraToElastic",	SSI_TYPE_CHECKBOX},
	{"loraToLoRa",		SSI_TYPE_CHECKBOX},
};

static esp_err_t handler(httpd_req_t *req) {
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	return httpRespond(req, pageStart, pageEnd, (ssiTag_t *) &ssiTags, sizeof(ssiTags) / sizeof(ssiTags[0]));
}

static httpd_uri_t getURI = {
    .uri      = "/config_lora.html",
    .method   = HTTP_GET,
    .handler  = handler
};

static httpd_uri_t postURI = {
    .uri      = "/config_lora.html",
    .method   = HTTP_POST,
    .handler  = handler
};



void httpPageConfigLoRaHTMLInit(httpd_handle_t server) {
	httpd_register_uri_handler(server, &getURI);
	httpd_register_uri_handler(server, &postURI);
}
