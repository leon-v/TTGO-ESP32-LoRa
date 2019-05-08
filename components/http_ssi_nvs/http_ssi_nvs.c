#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

#include "http.h"

#define TAG "httpSSINVS"

void httpSSINVSSetBit(nvs_handle nvsHandle, char * nvsKey, int bit, char * postValue){

	uint8_t value;

	nvs_get_u8(nvsHandle, nvsKey, &value);

	if (atoi(postValue)) {
		value|= (0x01 << bit);
	}
	else{
		value&= ~(0x01 << bit);
	}

	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, nvsKey, value));
}
void httpSSINVSGetBit(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint8_t value;
	char intValStr[4];
	nvs_get_u8(nvsHandle, nvsKey, &value);

	value = (value >> bit) & 0x01;

	itoa(value, intValStr, 10);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
}

void httpSSINVSGetChecked(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint8_t value;
	char intValStr[4];
	nvs_get_u8(nvsHandle, nvsKey, &value);

	value = (value >> bit) & 0x01;

	if (value){
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "checked"));
	}
}

void httpSSINVSSetInt32(nvs_handle nvsHandle, char * nvsKey, char * postValue){
	nvs_set_u32(nvsHandle, nvsKey, atoi(postValue));
}

void httpSSINVSGetInt32(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){
	uint32_t value;
	char intValStr[32];

	nvs_get_u32(nvsHandle, nvsKey, &value);

	itoa(value, intValStr, 10);

	ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, intValStr));
}

void httpSSINVSSetString(nvs_handle nvsHandle, char * nvsKey, char * value){
	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, nvsKey, value));
}

void httpSSINVSGetString(httpd_req_t *req, nvs_handle nvsHandle, char * nvsKey){
	size_t nvsLength = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;
	char strVal[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];

	nvs_get_str(nvsHandle, nvsKey, strVal, &nvsLength);

	if (nvsLength > 0){
		ESP_ERROR_CHECK(httpd_resp_send_chunk(req, strVal, nvsLength - 1));
	}

}

void httpSSINVSGet(httpd_req_t *req, char * ssiTag){

	char * nvsName = strtok(ssiTag, ":");
	char * error = NULL;
	if (!nvsName) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS name"));
		return;
	}

	char * nvsType = strtok(NULL, ":");
	if (!nvsType) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS type"));
		return;
	}

	char * nvsKey = strtok(NULL, ":");
	if (!nvsKey) {
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Missing NVS key"));
		return;
	}

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open(nvsName, NVS_READONLY, &nvsHandle));

	if (strcmp(nvsType, "string") == 0){
		httpSSINVSGetString(req, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "int") == 0){
		httpSSINVSGetInt32(req, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "bit") == 0){

		char * bitStr = strtok(NULL, ":");
		httpSSINVSGetBit(req, nvsHandle, nvsKey, atoi(bitStr));
	}
	else if (strcmp(nvsType, "checked") == 0){

		char * bitStr = strtok(NULL, ":");
		httpSSINVSGetChecked(req, nvsHandle, nvsKey, atoi(bitStr));
	}
	else{
		ESP_ERROR_CHECK(httpd_resp_sendstr_chunk(req, "Failed to parse NVS type"));
	}

	nvs_close(nvsHandle);
}


void httpSSINVSSet(char * ssiTag, char * value){

	char * nvsName = strtok(ssiTag, ":");
	if (!nvsName) {
		ESP_LOGE(TAG, "Missing NVS name");
		return;
	}

	char * nvsType = strtok(NULL, ":");
	if (!nvsType) {
		ESP_LOGE(TAG, "Missing NVS type");
		return;
	}

	char * nvsKey = strtok(NULL, ":");
	if (!nvsKey) {
		ESP_LOGE(TAG, "Missing NVS key");
		return;
	}


	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open(nvsName, NVS_READWRITE, &nvsHandle));

	if (strcmp(nvsType, "string") == 0){
		httpSSINVSSetString(nvsHandle, nvsKey, value);
	}
	else if (strcmp(nvsType, "int") == 0){
		httpSSINVSSetInt32(nvsHandle, nvsKey, value);
	}
	else if (strcmp(nvsType, "bit") == 0){

		char * bitStr = strtok(NULL, ":");
		httpSSINVSSetBit(nvsHandle, nvsKey, atoi(bitStr), value);
	}

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}