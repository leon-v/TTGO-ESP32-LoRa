#include <string.h>
#include <esp_log.h>
#include <nvs.h>

#include "wifi.h"
#include "mqtt_connection.h"
#include "message.h"

#define TAG "WiFi"

#define IDLE_CONFIG_BIT 0

static EventGroupHandle_t wifiEventGroup;
static char wifiEnabled = 0;
static TimerHandle_t idleTimer;

EventGroupHandle_t wifiGetEventGroup(void){
	return wifiEventGroup;
}

int wifiIdleDisconnectEnabled(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	unsigned char idleDisable;
	ESP_ERROR_CHECK(nvs_get_u8(nvsHandle, "idleDisable", &idleDisable));

	nvs_close(nvsHandle);

	return (idleDisable >> IDLE_CONFIG_BIT) & 0x01;
}

void wifiIdleTimeout( TimerHandle_t xTimer ){

	if (wifiEnabled){

		ESP_LOGW(TAG, "WiFi idle timer triggered.");

		if (!wifiIdleDisconnectEnabled()){
			return;
		}

		ESP_LOGW(TAG, "Disabling WiFi");

		mqttConnectionWiFiDisconnected();

    	ESP_ERROR_CHECK(esp_wifi_stop());

    	wifiEnabled = 0;
	}
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

	// ESP_LOGW(TAG, "WiFi Idle Timer Reset");
}

static void wifiGotIP(const ip4_addr_t *addr){

	message_t message;
	message.valueType = MESSAGE_STRING;

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength = sizeof(message.deviceName);
	nvs_get_str(nvsHandle, "uniqueName", message.deviceName, &nvsLength);

	nvs_close(nvsHandle);

	strcpy(message.sensorName, "wifiIp");
	strcpy(message.stringValue, ip4addr_ntoa(addr));

	messageIn(&message, TAG);
}

static esp_err_t wifiEventHandler(void *ctx, system_event_t *event){

	switch(event->event_id) {

		case SYSTEM_EVENT_STA_START:
			ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
			esp_wifi_connect();
        	break;

    	case SYSTEM_EVENT_STA_GOT_IP:
    		wifiGotIP(&event->event_info.got_ip.ip_info.ip);
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
    idleTimer = xTimerCreate("wiFiIdle", 60000 / portTICK_RATE_MS, pdTRUE, (void *) timerId, wifiIdleTimeout);

	if (xTimerStart(idleTimer, 0) != pdPASS) {
		ESP_LOGE(TAG, "Timer %d start error", timerId);
	}
}

void wifiResetNVS(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	unsigned char idleDisable;
	nvs_get_u8(nvsHandle, "idleDisable", &idleDisable);
	idleDisable&= ~(0x01 << IDLE_CONFIG_BIT); // Clear bit IDLE_CONFIG_BIT
	ESP_ERROR_CHECK(nvs_set_u8(nvsHandle, "idleDisable", idleDisable));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);

	messageNVSReset("wifi", 0xFF);
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