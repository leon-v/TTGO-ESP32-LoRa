#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#include "http.h"

#define TAG "httpSSIFunc"


void httpSSIFunctionsGet(httpd_req_t *req, char * ssiTag){

	if (strcmp(ssiTag, "runTimeStats") == 0){
		char buffer[1024];

		vTaskGetRunTimeStats(buffer);

		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, buffer));
	}
}