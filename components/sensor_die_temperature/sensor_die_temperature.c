
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
#include <time.h>
#include <nvs_flash.h>
#include <esp_system.h>


#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

static const char *TAG = "Sensor";

#include "message.h"
#include "radio.h"
#include "mqtt_connection.h"
#include "elastic.h"
#include "ssd1306.h"

esp_adc_cal_characteristics_t *adc_chars = NULL;

unsigned char disaplyLine = 0;

int sensorDieTemperatureRead (void) {
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


void sensorDieTemperatureSendMessage(char * sensor, float value) {
	message_t message;
	esp_err_t espError;

	char valueString[32] = {0};
	sprintf(valueString, "%.2f", value);

	ssd1306Text_t ssd1306Text;
	ssd1306Text.line = disaplyLine++;

	strcpy(ssd1306Text.text, sensor);
	strcat(ssd1306Text.text, ": ");
	strcat(ssd1306Text.text, valueString);

	ssd1306QueueText(&ssd1306Text);

	// Set device unique ID
    nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	size_t nvsLength = sizeof(message.deviceName);
	espError = nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

	nvs_close(nvsHandle);

	strcpy(message.sensorName, sensor);

	message.valueType = MESSAGE_FLOAT;
	message.floatValue = value;

	xQueueSend(radioGetQueue(), &message, 0);


	/*
	mqttConnectionMessage_t mqttConnectionMessage;

	strcpy(mqttConnectionMessage.topic, sensor);
	strcpy(mqttConnectionMessage.value, valueString);

	if (uxQueueSpacesAvailable(getMQTTConnectionMessageQueue())) {
		xQueueSend(getMQTTConnectionMessageQueue(), &mqttConnectionMessage, 0);
	}

	elasticMessage_t elasticMessage;

	strcpy(elasticMessage.name, sensor);
	elasticMessage.value = value;
	time(&elasticMessage.time);

	if (uxQueueSpacesAvailable(getSlasticMessageQueue())) {
		xQueueSend(getSlasticMessageQueue(), &elasticMessage, 0);
	}
	*/
}

float readADC(adc1_channel_t channel) {

	float result = 0.00;
	int count = 10000;

	for (int i = 0; i < count; i++) {
		// result+= esp_adc_cal_raw_to_voltage(adc1_get_raw(channel), adc_chars);
		result+= adc1_get_raw(channel);

	}

	result = result / count;

	return result;
}

static void sensorDieTemperatureTask(void *arg){

	EventBits_t eventBits;
	char sensor[32];

	while(1) {

		ESP_LOGI(TAG, "task start\n");

		vTaskDelay(2); // Delay for 2 ticks to allow scheduler time to execute

		ESP_LOGI(TAG, "Wait ended\n");

		vTaskDelay(2000 / portTICK_RATE_MS); // Delay for 2 ticks to allow scheduler time to execute

		ESP_LOGI(TAG, "Wait was connected\n");

		disaplyLine = 0;


		strcpy(sensor, "DieTemperature");

		int temperature = sensorDieTemperatureRead();
		float celcius = (temperature  - 32) / 1.8;


		sensorDieTemperatureSendMessage(sensor, celcius);



		strcpy(sensor, "HallSensor");

		float hall = hall_sensor_read();

		sensorDieTemperatureSendMessage(sensor, hall);



		strcpy(sensor, "Battery");

		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
		float adc11 = readADC(ADC1_CHANNEL_0) * 2;

		sensorDieTemperatureSendMessage(sensor, adc11);
	}
}



void sensorDieTemperatureInit(void) {

	// xTaskCreate(&sensorDieTemperatureTask, "sensorDieTemperature", 4096, NULL, 13, NULL);
}
