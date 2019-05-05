#include <nvs_flash.h>
#include <string.h>
#include <esp_log.h>

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
void httpSSINVSGetBit(char * tag, nvs_handle nvsHandle, char * nvsKey, int bit){
	uint8_t value;

	nvs_get_u8(nvsHandle, nvsKey, &value);

	value = (value >> bit) & 0x01;

	itoa(value, tag, 10);
}

void httpSSINVSSetInt32(nvs_handle nvsHandle, char * nvsKey, char * postValue){
	nvs_set_u32(nvsHandle, nvsKey, atoi(postValue));
}

void httpSSINVSGetInt32(char * tag, nvs_handle nvsHandle, char * nvsKey){
	uint32_t value;

	nvs_get_u32(nvsHandle, nvsKey, &value);

	itoa(value, tag, 10);
}

void httpSSINVSSetString(nvs_handle nvsHandle, char * nvsKey, char * value){
	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, nvsKey, value));
}

void httpSSINVSGetString(char * tag, nvs_handle nvsHandle, char * nvsKey){
	size_t nvsLength = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;

	nvs_get_str(nvsHandle, nvsKey, tag, &nvsLength);
}

void httpSSINVSGet(char * tag, char * ssiTag){

	char * nvsName = strtok(ssiTag, ":");
	if (!nvsName) {
		strcpy(tag, "Missing NVS name");
		return;
	}

	char * nvsType = strtok(NULL, ":");
	if (!nvsType) {
		strcpy(tag, "Missing NVS type");
		return;
	}

	char * nvsKey = strtok(NULL, ":");
	if (!nvsKey) {
		strcpy(tag, "Missing NVS key");
		return;
	}

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open(nvsName, NVS_READONLY, &nvsHandle));

	if (strcmp(nvsType, "string") == 0){
		httpSSINVSGetString(tag, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "int") == 0){
		httpSSINVSGetInt32(tag, nvsHandle, nvsKey);
	}
	else if (strcmp(nvsType, "bit") == 0){

		char * bitStr = strtok(NULL, ":");
		httpSSINVSGetBit(tag, nvsHandle, nvsKey, atoi(bitStr));
	}
	else if (strcmp(nvsType, "checked") == 0){

		char * bitStr = strtok(NULL, ":");
		httpSSINVSGetBit(tag, nvsHandle, nvsKey, atoi(bitStr));

		if (strcmp(tag, "1") == 0){
			strcpy(tag, "checked");
		}
		else{
			strcpy(tag, "");
		}
	}
	else{
		strcpy(tag, "Failed to parse NVS type");
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