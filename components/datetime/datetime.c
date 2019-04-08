#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/apps/sntp.h"

#include "wifi.h"
#include "datetime.h"

static EventGroupHandle_t datetimeEventGroup;

static const char *TAG = "datetime";

EventGroupHandle_t dateTimeGetEventGroup(void){
	return datetimeEventGroup;
}

static void dateTimeTask(void *arg){

	xEventGroupClearBits(datetimeEventGroup, TIME_READY_BIT);

	xEventGroupWaitBits(wifiGetEventGroup(), WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

    size_t nvsLength;

	char dateTimeNTPHost[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
    nvsLength = sizeof(dateTimeNTPHost);
	nvs_get_str(nvsHandle, "dateTimeNTPHost", dateTimeNTPHost, &nvsLength);

	nvs_close(nvsHandle);

    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, dateTimeNTPHost);
    sntp_init();

    // wait for time to be set
	time_t now = 0;
	struct tm timeinfo = { 0 };

    while(timeinfo.tm_year < (2016 - 1900)) {
    	vTaskDelay(2); // Delay for 2 ticks to allow scheduler time to execute
    	ESP_LOGI(TAG, "Waiting for time to get updated from %s", dateTimeNTPHost);
    	time(&now);
    	localtime_r(&now, &timeinfo);
        vTaskDelay(4000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Time ready.");

    xEventGroupSetBits(datetimeEventGroup, TIME_READY_BIT);

    vTaskDelete(NULL);
}

void dateTimeInit(void){

	datetimeEventGroup = xEventGroupCreate();

	xTaskCreate(&dateTimeTask, "dateTimeTask", 2048, NULL, 13, NULL);

}
void datTimeResetNVS(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "dateTimeNTPHost", "pool.ntp.org"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);

}