#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>
#include <driver/gpio.h>

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

// #include "os.h"
// #include "sys/param.h"
// #include "crypto/base64.h"

void app_main(void) {

	esp_err_t espError;

    //Initialize NVS
	espError = nvs_flash_init();

    if (espError == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
      	ESP_ERROR_CHECK(nvs_flash_init());
	}

	// beelineInit();

	// Setup config button
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (0b1 << 0);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    printf("Waiting for GIO0 to go low to start config mode....\n");
    ets_delay_us(3000000);

    if (gpio_get_level(0)){
    	printf("Starting in normal mode.\n");
    	wifiClientInit();
    }
    else{

    	printf("Starting in config mode. Erasing all NVS settings.\n");
    	// Reset all NVS data so we always get known values and don't crash
    	wifiClientResetNVS();
    	mqttConnectionResetNVS();
    	elasticResetNVS();
    	datTimeResetNVS();

    	wifiAccessPointInit();
    }

    httpServerInit();
    mqttConnectionInit();

    elasticInit();

    // radioInit();

    dateTimeInit();

    sensorDieTemperatureInit();

    ssd1306Init();
}