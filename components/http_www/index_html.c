#include "http.h"


extern const char  pageStart[]	asm("_binary_index_html_start");
extern const char  pageEnd[]	asm("_binary_index_html_end");

esp_err_t handler(httpd_req_t *req) {
	httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
	return httpRespond(req, pageStart, pageEnd, NULL, 0);
}

httpd_uri_t getURI = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = handler
};

void httpPageIndexHTMLInit(httpd_handle_t server) {
	httpd_register_uri_handler(server, &getURI);
}