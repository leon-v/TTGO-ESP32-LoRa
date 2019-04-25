#include <string.h>
#include <esp_log.h>

#include "wifi.h"
#include "mqtt_connection.h"

#define TAG "WiFi"

static EventGroupHandle_t wifiEventGroup;
static char wifiEnabled = 0;
static TimerHandle_t idleTimer;

EventGroupHandle_t wifiGetEventGroup(void){
	return wifiEventGroup;
}

void wifiIdleTimeout( TimerHandle_t xTimer ){

	if (wifiEnabled){

		mqttConnectionWiFiDisconnected();

    	ESP_LOGW(TAG, "WiFi idle timer triggered. Disabling WiFi");

    	ESP_ERROR_CHECK(esp_wifi_stop());

    	wifiEnabled = 0;
	}

	ESP_LOGW(TAG, "WiFi Idle Timer Triggered");
}

void wifiUsed(void){

	if (xTimerReset(idleTimer, 0) != pdPASS) {
		ESP_LOGE(TAG, "Timer reset error");
	}

	if (!wifiEnabled){

		ESP_ERROR_CHECK(esp_wifi_start());

		mqttConnectionWiFiConnected();

    	ESP_LOGW(TAG, "WiFi idle timer reset. Enabling WiFi");

    	wifiEnabled = 1;
	}

	ESP_LOGW(TAG, "WiFi Idle Timer Reset");
}

static esp_err_t wifiEventHandler(void *ctx, system_event_t *event){

	switch(event->event_id) {

		case SYSTEM_EVENT_STA_START:
			ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
			esp_wifi_connect();
        	break;

    	case SYSTEM_EVENT_STA_GOT_IP:
    		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        	xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        	break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        	ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        	xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        	esp_wifi_connect();
        	break;

		case SYSTEM_EVENT_AP_STACONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED "MACSTR, MAC2STR(event->event_info.sta_connected.mac));
			xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STADISCONNECTED "MACSTR, MAC2STR(event->event_info.sta_disconnected.mac));
			break;

		case SYSTEM_EVENT_AP_START:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
			break;

		case SYSTEM_EVENT_AP_STOP:
			ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");
			break;

		default:
			break;
	}

	return ESP_OK;
}

void wifiInit(void) {

	printf("WiFi - Initialisation - Start.\n");

	wifiEventGroup = xEventGroupCreate();

	tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_init(wifiEventHandler, NULL));

    int timerId = 1;
    idleTimer = xTimerCreate("wiFiIdle", 30000 / portTICK_RATE_MS, pdTRUE, (void *) timerId, wifiIdleTimeout);

	if (xTimerStart(idleTimer, 0) != pdPASS) {
		ESP_LOGE(TAG, "Timer %d start error", timerId);
	}
}



// void wifiStationScanDone(void *arg, int status){

// 	struct bss_info *bss = (struct bss_info *) arg;

//     while (bss != NULL) {

//         if (bss->channel != 0) {

//             struct router_info *info = NULL;
//             os_printf("ssid %s, channel %d, authmode %d, rssi %d\n", bss->ssid, bss->channel, bss->authmode, bss->rssi);
//         }
//         bss = bss->next.stqe_next;
//     }
// }

// void wifiStationScanStart(void){

// 	wifi_set_opmode(STATION_MODE);
// 	wifi_station_set_auto_connect(0);

// 	wifi_station_scan(NULL, wifiStationScanDone);
// }