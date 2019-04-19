
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

static const char * TAG = "dieSensors";

static const char * ROUTE_NAME = "dieSens";

#include "die_sensors.h"
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

static void dieSensorsTemperatureTask(void * arg){

	message_t message;
	message.valueType = MESSAGE_FLOAT;
	strcpy(message.sensorName, "DieTemp");

	size_t nvsLength;
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	unsigned char dieSensEn;
	unsigned int delay;

	while(1) {

		nvs_get_u8(nvsHandle, "dieSensEn", &dieSensEn);

		if (!((dieSensEn >> TEMPERATURE) & 0x01)) {
			vTaskDelay(4000 / portTICK_RATE_MS);
			continue;
		}

		nvs_get_u32(nvsHandle, "delayTemp", &delay);

		vTaskDelay(delay / portTICK_RATE_MS);

		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
		message.floatValue = dieSensorsReadADC(ADC1_CHANNEL_0) * 2;

		nvsLength = sizeof(message.deviceName);
		nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

		messageIn(&message, ROUTE_NAME);
	}

	nvs_close(nvsHandle);

	vTaskDelete(NULL);
}

static void dieSensorsHallEffectTask(void * arg){

	message_t message;
	message.valueType = MESSAGE_FLOAT;
	strcpy(message.sensorName, "Hall");

	size_t nvsLength;
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	unsigned char dieSensEn;
	unsigned int delay;

	while(1) {

		nvs_get_u8(nvsHandle, "dieSensEn", &dieSensEn);

		if (!((dieSensEn >> HALL_EFFECT) & 0x01)) {
			vTaskDelay(4000 / portTICK_RATE_MS);
			continue;
		}

		nvs_get_u32(nvsHandle, "delayHall", &delay);

		vTaskDelay(delay / portTICK_RATE_MS);

		message.floatValue = hall_sensor_read();

		nvsLength = sizeof(message.deviceName);
		nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

		messageIn(&message, ROUTE_NAME);
	}

	nvs_close(nvsHandle);

	vTaskDelete(NULL);
}

static void dieSensorsBatteryVoltageTask(void * arg){

	message_t message;
	message.valueType = MESSAGE_FLOAT;
	strcpy(message.sensorName, "Battery");

	size_t nvsLength;
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	unsigned char dieSensEn;
	unsigned int delay;

	while(1) {

		nvs_get_u8(nvsHandle, "dieSensEn", &dieSensEn);

		if (!((dieSensEn >> BATTERY_VOLTAGE) & 0x01)) {
			vTaskDelay(4000 / portTICK_RATE_MS);
			continue;
		}

		nvs_get_u32(nvsHandle, "delayBattV", &delay);

		vTaskDelay(delay / portTICK_RATE_MS);

		message.floatValue = hall_sensor_read();

		nvsLength = sizeof(message.deviceName);
		nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

		messageIn(&message, ROUTE_NAME);
	}

	nvs_close(nvsHandle);

	vTaskDelete(NULL);
}

void dieSensorsInit(void) {
	// xTaskCreate(&dieSensorsTask, "dieSensors", 4096, NULL, 10, NULL);

	xTaskCreate(&dieSensorsTemperatureTask, "dieSensorsTemp", 4096, NULL, 10, NULL);
	xTaskCreate(&dieSensorsHallEffectTask, "dieSensorsHall", 4096, NULL, 10, NULL);
	xTaskCreate(&dieSensorsBatteryVoltageTask, "dieSensorsBattyV", 4096, NULL, 10, NULL);
}

void dieSensorsResetNVS(void) {

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, "dieSensEn", 0x00));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "delayTemp", 4000));
	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "delayHall", 4000));
	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "delayBattV", 4000));

	nvs_close(nvsHandle);

	messageNVSReset("dieSens");
}