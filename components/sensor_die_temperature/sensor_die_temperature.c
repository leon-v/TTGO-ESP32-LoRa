
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


#include "rom/ets_sys.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

static const char *TAG = "Sensor";

#include "mqtt_connection.h"
#include "elastic.h"
#include "ssd1306.h"

esp_adc_cal_characteristics_t *adc_chars = NULL;

unsigned char disaplyLine = 0;

// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

 // Converts a given integer x to string str[].  d is the number
 // of digits required in output. If d is more than the number
 // of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

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

	char valueString[32];
	ftoa(value, valueString, 1);

	ssd1306Text_t ssd1306Text;
	ssd1306Text.line = disaplyLine++;

	char disaplyMessage[32];
	strcpy(disaplyMessage, sensor);
	strcat(disaplyMessage, ": ");
	strcat(disaplyMessage, valueString);
	ssd1306Text.text = disaplyMessage;

	ssd1306SetText(&ssd1306Text);

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
}

float readADC(adc1_channel_t channel) {

	float result = 0.00;
	int count = 1000;

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

		eventBits = xEventGroupWaitBits(getMQTTConnectionEventGroup(), MQTT_CONNECTED_BIT, false, true, 4000 / portTICK_RATE_MS);

		ESP_LOGI(TAG, "Wait ended\n");

		if (!(eventBits & MQTT_CONNECTED_BIT)) {
			continue;
		}

		vTaskDelay(2000 / portTICK_RATE_MS); // Delay for 2 ticks to allow scheduler time to execute

		ESP_LOGI(TAG, "Wait was connected\n");

		disaplyLine = 0;

		strcpy(sensor, "DieTemperature");

		int temperature = sensorDieTemperatureRead();
		float celcius = (temperature  - 32) / 1.8;


		sensorDieTemperatureSendMessage(sensor, celcius);



		strcpy(sensor, "DieHall");

		float hall = hall_sensor_read();

		sensorDieTemperatureSendMessage(sensor, hall);



		strcpy(sensor, "ADC10");

		adc1_config_width(ADC_WIDTH_BIT_12);
		adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
		float adc11 = readADC(ADC1_CHANNEL_0) * 2;

		sensorDieTemperatureSendMessage(sensor, adc11);
	}
}



void sensorDieTemperatureInit(void) {

	xTaskCreate(&sensorDieTemperatureTask, "sensorDieTemperature", 4096, NULL, 13, NULL);
}
