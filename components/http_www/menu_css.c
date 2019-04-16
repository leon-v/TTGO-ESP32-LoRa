#include "http.h"

static const char  pageStart[]	asm("_binary_menu_css_start");
static const char  pageEnd[]	asm("_binary_menu_css_end");

static esp_err_t handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/css");
	return httpRespond(req, pageStart, pageEnd, NULL, 0);
}

static httpd_uri_t getURI = {
    .uri      = "/menu.css",
    .method   = HTTP_GET,
    .handler  = handler
};

void httpPageMenuCSSInit(httpd_handle_t server) {
	httpd_register_uri_handler(server, &getURI);
}
