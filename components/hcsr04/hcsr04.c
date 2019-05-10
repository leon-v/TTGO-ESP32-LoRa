#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <esp_log.h>

#define TRIG_PIN 13
#define ECHO_PIN 14

#define TAG "HCSR04"

static TimerHandle_t timer;
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
static xQueueHandle hcsr04Queue = NULL;

void hcsr04TimerExpire( TimerHandle_t xTimer ){

	int ticks = -1;

	xQueueSend(hcsr04Queue, &ticks, 0);
}

static void hcsr04ISR(void * arg){

	int state = (uint32_t) arg;

	xTimerStopFromISR( timer, &xHigherPriorityTaskWoken);

	int ticks = xTimerGetPeriod(timer);

	xQueueSendFromISR(hcsr04Queue, &ticks, 0);
}

void hcsr04Trigger(void){

	if (xTimerReset(timer, 0) != pdPASS) {
		ESP_LOGE(TAG, "Timer reset error");
		return;
	}

	gpio_set_level(TRIG_PIN, 1);
	vTaskDelay(1 / portTICK_RATE_MS);
	gpio_set_level(TRIG_PIN, 0);

	if( xTimerStart( timer, 0 ) != pdPASS ) {
		ESP_LOGE(TAG, "Timer start error");
		return;
	}
}

void hcsr04Task(void * arg){

	while (1){

		vTaskDelay(5000 / portTICK_RATE_MS);

		hcsr04Trigger();

		int ticks;
		if (!xQueueReceive(hcsr04Queue, &ticks, 500 / portTICK_RATE_MS)) {
			continue;
		}

		ESP_LOGW(TAG, "%d", ticks);
	}
}

void hcsr04Init(void){

	hcsr04Queue = xQueueCreate(1, sizeof(int));

	gpio_config_t io_conf;

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //set as output mode
    io_conf.mode = GPIO_MODE_INPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (0x01 << ECHO_PIN);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (0x01 << TRIG_PIN);
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(ECHO_PIN, hcsr04ISR, (void*) ECHO_PIN);

    int timerId = 1;
    timer = xTimerCreate("hcsr04Timer", 50 / portTICK_RATE_MS, pdFALSE, (void *) timerId, hcsr04TimerExpire);
    xTimerStop(timer, 0);

    xTaskCreate(&hcsr04Task, "hcsr04Task", 2048, NULL, 20, NULL);
}
