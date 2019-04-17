
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <nvs_flash.h>
#include <esp_system.h>

#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

static const char *TAG = "dieSensors";

#include "message.h"

esp_adc_cal_characteristics_t *adc_chars = NULL;

// unsigned char disaplyLine = 0;

static int dieSensorsGetTemperature (void) {
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR, 3, SENS_FORCE_XPD_SAR_S);
    SET_PERI_REG_BITS(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_CLK_DIV, 10, SENS_TSENS_CLK_DIV_S);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    CLEAR_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP_FORCE);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_POWER_UP);
    ets_delay_us(100);
    SET_PERI_REG_MASK(SENS_SAR_TSENS_CTRL_REG, SENS_TSENS_DUMP_OUT);
    ets_delay_us(5);
    return GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR3_REG, SENS_TSENS_OUT, SENS_TSENS_OUT_S);
}


static void dieSensorsSendMessage(char * sensor, float value) {
	message_t message;
	esp_err_t espError;

	// Set device unique ID
    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength = sizeof(message.deviceName);
	espError = nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

	nvs_close(nvsHandle);

	strcpy(message.sensorName, sensor);

	message.valueType = MESSAGE_FLOAT;
	message.floatValue = value;

	messageIn(&message, "dieSens");
}

static float dieSensorsReadADC(adc1_channel_t channel) {

	float result = 0.00;
	int count = 10000;

	for (int i = 0; i < count; i++) {
		// result+= esp_adc_cal_raw_to_voltage(adc1_get_raw(channel), adc_chars);
		result+= adc1_get_raw(channel);

	}

	result = result / count;

	return result;
}


static void dieSensorsTask(void *arg){

	ESP_LOGI(TAG, "task start\n");

	while(1) {

		// disaplyLine = 0;


		vTaskDelay(5000 / portTICK_RATE_MS);

		int temperature = dieSensorsGetTemperature();
		float celcius = (temperature  - 32) / 1.8;
		dieSensorsSendMessage("DieTemp", celcius);


		vTaskDelay(5000 / portTICK_RATE_MS);

		float hall = hall_sensor_read();
		dieSensorsSendMessage("Hall", hall);


		vTaskDelay(5000 / portTICK_RATE_MS);

		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
		float adc11 = dieSensorsReadADC(ADC1_CHANNEL_0) * 2;

		dieSensorsSendMessage("Battery", adc11);
	}
}

void dieSensorsInit(void) {
	xTaskCreate(&dieSensorsTask, "dieSensors", 4096, NULL, 10, NULL);
}

void dieSensorsResetNVS(void) {
	messageNVSReset("dieSens");
}