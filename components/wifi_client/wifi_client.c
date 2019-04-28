#include <esp_wifi.h>
#include <nvs.h>
#include <esp_log.h>

#include "wifi.h"

#define TAG "WiFiClient"

void wifiClientInit(void) {

	wifiInit();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    wifi_config_t wifi_config = {
	    .sta = {
	        .ssid = "",
	        .password = ""
	    },
	};

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));



    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength;

	nvsLength = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;
	nvs_get_str(nvsHandle, "wifiSSID", (char *) wifi_config.sta.ssid, &nvsLength);

    nvsLength = CONFIG_HTTP_NVS_MAX_STRING_LENGTH;
    nvs_get_str(nvsHandle, "wifiPassword", (char *) wifi_config.sta.password, &nvsLength);

    nvs_close(nvsHandle);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

	ESP_LOGI(TAG, "WiFI Connecting to AP");
}

void wifiClientResetNVS(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "wifiSSID", "Wifi"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "wifiPassword", "password"));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}