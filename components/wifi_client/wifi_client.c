#include <esp_wifi.h>
#include <nvs.h>
#include <esp_pm.h>

#include "wifi.h"

#define MAX_CONFIG_STRING_LENGTH 32

void wifiClientInit(void) {

	#if CONFIG_PM_ENABLE
	    // Configure dynamic frequency scaling:
	    // maximum and minimum frequencies are set in sdkconfig,
	    // automatic light sleep is enabled if tickless idle support is enabled.
	    esp_pm_config_esp32_t pm_config = {
	            .max_freq_mhz = 240,
	            .min_freq_mhz = 80,
		#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
	            .light_sleep_enable = true
		#endif
    	};
    	ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
	#endif // CONFIG_PM_ENABLE

	wifiInit();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = ""
        },
    };

    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength;

	nvsLength = MAX_CONFIG_STRING_LENGTH;
	nvs_get_str(nvsHandle, "wifiSSID", (char *) wifi_config.sta.ssid, &nvsLength);

    nvsLength = MAX_CONFIG_STRING_LENGTH;
    nvs_get_str(nvsHandle, "wifiPassword", (char *) wifi_config.sta.password, &nvsLength);

    nvs_close(nvsHandle);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

    printf ("WiFI connect to ap \n");
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