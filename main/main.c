#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_pm.h>

//#include "beeline.h"

#include "wifi.h"
#include "wifi_access_point.h"
#include "wifi_client.h"

#include "http.h"
#include "mqtt_connection.h"
#include "die_sensors.h"
#include "elastic.h"
#include "display.h"
// #include "radio.h"
#include "datetime.h"
#include "ssd1306.h"
#include "radio.h"
#include "hcsr04.h"

// #include "os.h"
// #include "sys/param.h"
// #include "crypto/base64.h"

static const char *TAG = "Main";

void app_main(void) {

	ssd1306Init();

	esp_err_t espError;

    //Initialize NVS
	espError = nvs_flash_init();

    if (espError == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
      	ESP_ERROR_CHECK(nvs_flash_init());
	}

	// Set device unique ID
    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	size_t nvsLength;
	espError = nvs_get_str(nvsHandle, "uniqueName", NULL, &nvsLength);

	if (espError == ESP_ERR_NVS_NOT_FOUND){
		uint8_t mac[6];
	    char id_string[8] = {0};
	    esp_read_mac(mac, ESP_MAC_WIFI_STA);

	    id_string[0] = 'a' + ((mac[3] >> 0) & 0x0F);
	    id_string[1] = 'a' + ((mac[3] >> 4) & 0x0F);
	    id_string[2] = 'a' + ((mac[4] >> 0) & 0x0F);
	    id_string[3] = 'a' + ((mac[4] >> 4) & 0x0F);
	    id_string[4] = 'a' + ((mac[5] >> 0) & 0x0F);
	    id_string[5] = 'a' + ((mac[5] >> 4) & 0x0F);
	    id_string[6] = 0;

	    ESP_LOGE(TAG, "id_string %s, B:%d, 1:%d, 2:%d, A1:%d, A2:%d",
	    	id_string,
	    	mac[3],
	    	((mac[3] >> 0) & 0x0F),
	    	((mac[3] >> 4) & 0x0F),
	    	id_string[0],
	    	id_string[1]
	    );

		ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "uniqueName", id_string));
		ESP_ERROR_CHECK(nvs_commit(nvsHandle));
		nvs_close(nvsHandle);
	}

	// Setup config button
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (0b1 << 0);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    int configMode = !gpio_get_level(0);

    if (configMode){

    	ESP_LOGW(TAG, "Booting in config mode");

    	// Reset all NVS data so we always get known values and don't crash
    	wifiResetNVS();
    	wifiClientResetNVS();
    	mqttConnectionResetNVS();
    	elasticResetNVS();
    	hcsr04ResetNVS();
    	datTimeResetNVS();
    	radioResetNVS();
    	dieSensorsResetNVS();
    	displayResetNVS();

    	wifiAccessPointInit();


    }
    else{

    	ESP_LOGW(TAG, "Booting...");

    	wifiClientInit();
    }

    #if CONFIG_PM_ENABLE
	    // Configure dynamic frequency scaling:
	    // maximum and minimum frequencies are set in sdkconfig,
	    // automatic light sleep is enabled if tickless idle support is enabled.
	    esp_pm_config_esp32_t pm_config = {
	            .max_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
	            .min_freq_mhz = CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ,
		#if CONFIG_FREERTOS_USE_TICKLESS_IDLE
	            .light_sleep_enable = true
		#endif
    	};
    	ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );
	#endif // CONFIG_PM_ENABLE

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL3);

    dateTimeInit();

    httpServerInit();

    mqttConnectionInit();

    displayInit();

    elasticInit();

    radioInit();

    dieSensorsInit();

    hcsr04Init();
}

