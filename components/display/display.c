#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <string.h>

#include "message.h"

#define TAG "display"


static xQueueHandle displayQueue = NULL;

static void displayTask(void * arg) {

	while (1){

		vTaskDelay(2); // Delay for 2 ticks to allow scheduler time to execute

		message_t message;
		if (xQueueReceive(displayQueue, &message, 4000 / portTICK_RATE_MS)) {
			ESP_LOGI(TAG, "Got Message");
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

	xTaskCreate(&displayTask, "displayTask", 4096, NULL, 10, NULL);
}