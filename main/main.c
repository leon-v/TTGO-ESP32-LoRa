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
#include "sensor_die_temperature.h"
#include "elastic.h"
// #include "radio.h"
#include "datetime.h"
#include "ssd1306.h"
#include "radio.h"

// #include "os.h"
// #include "sys/param.h"
// #include "crypto/base64.h"

static const char *TAG = "Main";

void app_main(void) {

	ssd1306Init();

	esp_err_t espError;
	ssd1306Text_t disaply;
	disaply.line = 0;

	strcpy(disaply.text, "Booting...");
	ssd1306QueueText(&disaply);


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

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    strcpy(disaply.text, "Hold PRG for config mode.");
    ssd1306QueueText(&disaply);
    ESP_LOGW(TAG, "%s", disaply.text);


    vTaskDelay(2000 / portTICK_PERIOD_MS);

    int configMode = 0;

    configMode = !gpio_get_level(0);

    if (configMode){

    	strcpy(disaply.text, "Starting in config mode.");

    	ssd1306QueueText(&disaply);

    	ESP_LOGW(TAG, "%s", disaply.text);

    	// Reset all NVS data so we always get known values and don't crash
    	wifiClientResetNVS();
    	mqttConnectionResetNVS();
    	elasticResetNVS();
    	datTimeResetNVS();
    	radioResetNVS();

    	wifiAccessPointInit();
    }
    else{

    	strcpy(disaply.text, "Starting in normal mode.");
    	ssd1306QueueText(&disaply);

    	ESP_LOGW(TAG, "%s", disaply.text);
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

    httpServerInit();

    radioInit();

    mqttConnectionInit();

    elasticInit();

    dateTimeInit();

    sensorDieTemperatureInit();
}

