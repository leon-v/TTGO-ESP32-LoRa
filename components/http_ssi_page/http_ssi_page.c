#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#include "http.h"

#define TAG "httpSSIPage"


static const char  template_top_html[]	asm("_binary_template_top_html_start");
static const char  template_bottom_html[]	asm("_binary_template_bottom_html_start");

void httpSSIPageGet(httpd_req_t *req, char * ssiTag){

	char * error = NULL;

	if (strcmp(ssiTag, "template_top_html") == 0){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, template_top_html));
	}

	else if (strcmp(ssiTag, "template_bottom_html") == 0){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, template_bottom_html));
	}

	else{
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "SSI Page tag not found "));
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, ssiTag));

	}

}