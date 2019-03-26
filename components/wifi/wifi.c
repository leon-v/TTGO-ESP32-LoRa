#include <string.h>

#include "wifi.h"

static EventGroupHandle_t wifiEventGroup;

EventGroupHandle_t wifiGetEventGroup(void){
	return wifiEventGroup;
}

static esp_err_t wifiEventHandler(void *ctx, system_event_t *event){

	switch(event->event_id) {

		case SYSTEM_EVENT_STA_START:
			printf("WiFi - Event - Station start.\n");
        	esp_wifi_connect();
        	break;

    	case SYSTEM_EVENT_STA_GOT_IP:
        	xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        	break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        	printf("WiFi - Event - Station disconnected.\n");
        	xEventGroupClearBits(wifiEventGroup, WIFI_CONNECTED_BIT);
        	esp_wifi_connect();
        	break;

		case SYSTEM_EVENT_AP_STACONNECTED:
			printf("WiFi - Event - Client connected to out access point: "MACSTR".\n", MAC2STR(event->event_info.sta_connected.mac));
			xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
			break;

		case SYSTEM_EVENT_AP_STADISCONNECTED:
			printf("WiFi - Event - Client disconnected to out access point: "MACSTR".\n", MAC2STR(event->event_info.sta_disconnected.mac));
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