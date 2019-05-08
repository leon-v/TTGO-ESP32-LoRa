#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#define TAG "httpSSIFunc"

void httpSSIFunctionsGet(char * outBuffer, char * ssiTag){

	if (strcmp(ssiTag, "runTimeStats") == 0){
		vTaskGetRunTimeStats(outBuffer);
	}
}