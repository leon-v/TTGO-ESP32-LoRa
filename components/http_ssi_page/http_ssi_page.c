#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>

#define TAG "httpSSIPage"


static const char  template_top_html[]	asm("_binary_template_top_html_start");
static const char  template_bottom_html[]	asm("_binary_template_bottom_html_start");

void httpSSIPageGet(char * outBuffer, char * ssiTag){

	if (strcmp(ssiTag, "template_top_html") == 0){
		strcpy(outBuffer, template_top_html);
	}

	else if (strcmp(ssiTag, "template_bottom_html") == 0){
		strcpy(outBuffer, template_bottom_html);
	}

	else{
		strcpy(outBuffer, ssiTag);
		strcat(outBuffer, " not found");
	}

}