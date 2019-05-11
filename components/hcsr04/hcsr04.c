#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/timers.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include <soc/timer_group_struct.h>
#include <driver/periph_ctrl.h>
#include <driver/timer.h>

#define TRIG_PIN 13
#define ECHO_PIN 12

#define TAG "HCSR04"

#define TIMER_DIVIDER         2  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_SCALEUS		  TIMER_SCALE * 1000000

static xQueueHandle hcsr04Queue = NULL;

unsigned char timerRunning = 0;

void IRAM_ATTR hcsr04TimerISR(void * arg){

	timer_pause(TIMER_GROUP_0, TIMER_0);

	TIMERG0.int_clr_timers.t0 = 1;

	uint64_t counts = 0;

	xQueueSendFromISR(hcsr04Queue, &counts, NULL);
}

void IRAM_ATTR hcsr04EchoISR(void * arg){

	timer_pause(TIMER_GROUP_0, TIMER_0);

	uint32_t pin = (uint32_t) arg;

	if (gpio_get_level(pin)){

		timer_start(TIMER_GROUP_0, TIMER_0);
	}
	else{

		uint64_t counts;
		timer_get_counter_value(TIMER_GROUP_0, TIMER_0, &counts);

		xQueueSendFromISR(hcsr04Queue, &counts, NULL);
	}
}

void hcsr04Trigger(void){

	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0x00000000ULL);

	timer_set_alarm(TIMER_GROUP_0, TIMER_0, TIMER_ALARM_EN);

	gpio_set_level(TRIG_PIN, 1);

	ets_delay_us(10);

	gpio_set_level(TRIG_PIN, 0);
}

#define SAMPLES 10;
void hcsr04Task(void * arg){

	while (1){

		vTaskDelay(1000 / portTICK_RATE_MS);

		int samples = SAMPLES;
		int actualSamples = 0;
		double distance = 0.00;
		uint64_t counts;
		double average = 0.00;
		double microseconds = 0.00;

		while (samples--){

			vTaskDelay(60 / portTICK_RATE_MS);

			hcsr04Trigger();

			if (!xQueueReceive(hcsr04Queue, &counts, 20 / portTICK_RATE_MS)) {
				// ESP_LOGE(TAG, " No response from module");
				continue;
			}

			if (counts == 0){
				continue;
			}

			actualSamples++;
			average+= counts;
		}

		if (actualSamples < 10){
			ESP_LOGE(TAG, "Not enough samples.");
			continue;
		}

		average/= actualSamples;

		average = average / TIMER_SCALEUS;

		distance = average / 5.8;

		ESP_LOGW(TAG, "Average Count: %f = %fus = %fmm\n", average, microseconds, distance);
	}
}

void hcsr04Init(void){

	hcsr04Queue = xQueueCreate(3, sizeof(double));

	gpio_config_t io_conf;

    //disable interrupt
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
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
    gpio_isr_handler_add(ECHO_PIN, hcsr04EchoISR, (void*) ECHO_PIN);
    // gpio_isr_register(hcsr04EchoISR, (void*) ECHO_PIN, ESP_INTR_FLAG_HIGH);


    /* Select and initialize basic parameters of the timer */
    timer_config_t config;
    config.auto_reload = TIMER_AUTORELOAD_DIS;
    config.divider = TIMER_DIVIDER;
    config.counter_dir = TIMER_COUNT_UP;
    config.counter_en = TIMER_PAUSE;
    config.intr_type = TIMER_INTR_LEVEL;
    config.auto_reload = 0;

    timer_init(TIMER_GROUP_0, TIMER_0, &config);

    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, 0.01 * TIMER_SCALE);

    timer_isr_register(TIMER_GROUP_0, TIMER_0, hcsr04TimerISR, NULL, ESP_INTR_FLAG_IRAM, NULL);

    timer_enable_intr(TIMER_GROUP_0, TIMER_0);

    xTaskCreate(&hcsr04Task, "hcsr04Task", 2048, NULL, 13, NULL);
}
