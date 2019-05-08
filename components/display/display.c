#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <string.h>

#include "message.h"
#include "ssd1306.h"

#define TAG "display"

static int messagesLength;
static message_t messages[16];
static char template[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
static char uniqueName[16];

static xQueueHandle displayQueue = NULL;

static void loadTemplate(void){
	size_t nvsLength;

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	nvsLength = sizeof(template);
	nvs_get_str(nvsHandle, "displayTemplate", template, &nvsLength);

	char uniqueName[16];
	nvsLength = sizeof(uniqueName);
	nvs_get_str(nvsHandle, "uniqueName", uniqueName, &nvsLength);

	nvs_close(nvsHandle);
}

static void updateVariables(void){

	messagesLength = 0;

	loadTemplate();

	char * token = strtok(template, "[");

	while (token != NULL){

		char * device = strtok(NULL, ":]");
		char * sensor = strtok(NULL, "]");

		if (!sensor){
			sensor = device;
			device = uniqueName;
		}

		if (sensor){

			if (messagesLength >= sizeof(messages)) {
				ESP_LOGE(TAG, "No room left in display value message buffer");
			}

			else{
				message_t * message = &messages[messagesLength];

				strcpy(message->deviceName, device);
				strcpy(message->sensorName, sensor);
				message->valueType = MESSAGE_STRING;
				strcpy(message->stringValue, "???");

				ESP_LOGE(TAG, "Found tag %s:%s", message->deviceName, message->sensorName);

				messagesLength++;
			}

		}

		token = strtok(NULL, "[");
	}

}

static void disaplyGetMessageValue(char * device, char * sensor, char * value) {

	for (int i = 0; i < messagesLength; i++){

		message_t * displayMessage = &messages[i];

		if (strcmp(displayMessage->deviceName, device) == 0) {
			if (strcmp(displayMessage->sensorName, sensor) == 0) {
				switch (displayMessage->valueType) {

					case MESSAGE_INT:
						sprintf(value, "%d", displayMessage->intValue);
					break;

					case MESSAGE_FLOAT:
						sprintf(value, "%.2f", displayMessage->floatValue);
					break;

					case MESSAGE_DOUBLE:
						sprintf(value, "%.4f", displayMessage->floatValue);
					break;

					case MESSAGE_STRING:
						strcpy(value, displayMessage->stringValue);
					break;
				}
			}
		}
	}
}
static void disaplyUpdate(void){

	loadTemplate();

	char * token = strtok(template, "[");
	char disaply[CONFIG_HTTP_NVS_MAX_STRING_LENGTH] = {0};

	while (token != NULL){

		strcat(disaply, token);

		char * device = strtok(NULL, ":]");
		char * sensor = strtok(NULL, "]");

		if (!sensor){
			sensor = device;
			device = &uniqueName;
		}

		if (sensor){

			char value[32];
			disaplyGetMessageValue(device, sensor, value);
			strcat(disaply, value);
		}

		token = strtok(NULL, "[");
	}

	ssd1306SetText(disaply);
}

static void updateVariable(message_t * message){

	for (int i = 0; i < messagesLength; i++) {

		message_t * displayMessage = &messages[i];

		if (strcmp(displayMessage->deviceName, message->deviceName) == 0) {
			if (strcmp(displayMessage->sensorName, message->sensorName) == 0) {

				displayMessage->valueType = message->valueType;
				switch (displayMessage->valueType) {

					case MESSAGE_INT:
						displayMessage->intValue = message->intValue;
					break;

					case MESSAGE_FLOAT:
						displayMessage->floatValue = message->floatValue;
					break;

					case MESSAGE_DOUBLE:
						displayMessage->doubleValue = message->doubleValue;
					break;

					case MESSAGE_STRING:
						strcpy(displayMessage->stringValue, message->stringValue);
					break;
				}
			}
		}
	}
}

static void displayTask(void * arg) {

	while (true) {

		vTaskDelay(2); // Delay for 2 ticks to allow scheduler time to execute

		message_t message;
		if (xQueueReceive(displayQueue, &message, 500 / portTICK_RATE_MS)) {
			updateVariable(&message);
			disaplyUpdate();
		}
	}
}

void displayQueueAdd(message_t * message){
	if (uxQueueSpacesAvailable(displayQueue)) {
		xQueueSend(displayQueue, message, 0);
	}
}

void displayInit(void){

	displayQueue = xQueueCreate(4, sizeof(message_t));

	loadTemplate();
	updateVariables();

	xTaskCreate(&displayTask, "displayTask", 4096, NULL, 11, NULL);
}

void displayResetNVS(void) {
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "displayTemplate", "Battery: [device:Battery]"));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}