#include <nvs.h>
#include <mqtt_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>

#include "wifi.h"
// #include "http.h"
#include "mqtt_connection.h"
#include "message.h"

static esp_mqtt_client_handle_t client;
static esp_mqtt_client_config_t mqtt_cfg;

static xQueueHandle mqttConnectionQueue = NULL;

static EventGroupHandle_t mqttConnectionEventGroup;

static const char *TAG = "MQTT";

static esp_err_t mqttConnectionEventHandler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
        	xEventGroupSetBits(mqttConnectionEventGroup, MQTT_CONNECTED_BIT);

   //      	beelineMessage_t beelineMessage;
   //      	strcpy(beelineMessage.name, "TestName");
			// strcpy(beelineMessage.sensor, "TestSensor");
			// strcpy(beelineMessage.value, "TestValue");

			// xQueueSend(mqttConnectionQueue, &beelineMessage, 0);

            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            // ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            // msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            // ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);


		break;

		case MQTT_EVENT_BEFORE_CONNECT:
			// wifiUsed();
		break;

        case MQTT_EVENT_DISCONNECTED:
        	xEventGroupClearBits(mqttConnectionEventGroup, MQTT_CONNECTED_BIT);
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;

        case MQTT_EVENT_SUBSCRIBED:
        	wifiUsed();
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
		break;

        case MQTT_EVENT_UNSUBSCRIBED:
        	wifiUsed();
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;

        case MQTT_EVENT_PUBLISHED:
        	wifiUsed();
            // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		break;

        case MQTT_EVENT_DATA:
        	wifiUsed();
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
		break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;

    }
    return ESP_OK;
}

void mqttConnectionSetClient(void){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

    size_t nvsLength;

    char host[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
    nvsLength = sizeof(host);
	nvs_get_str(nvsHandle, "mqttHost", host, &nvsLength);

	unsigned int port;
	nvs_get_u32(nvsHandle, "mqttPort", &port);

	char username[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
	nvsLength = sizeof(username);
	nvs_get_str(nvsHandle, "mqttUsername", username, &nvsLength);

	char password[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
	nvsLength = sizeof(password);
	nvs_get_str(nvsHandle, "mqttPassword", password, &nvsLength);

	unsigned int keepalive;
	nvs_get_u32(nvsHandle, "mqttKeepalive", &keepalive);

	char uniqueName[16];
	nvsLength = sizeof(uniqueName);
	nvs_get_str(nvsHandle, "uniqueName", uniqueName, &nvsLength);

	nvs_close(nvsHandle);

	ESP_LOGI(TAG, "Got MQTT host details...\n");

	mqtt_cfg.host = host;
    mqtt_cfg.port = port;
    mqtt_cfg.client_id = uniqueName;
    mqtt_cfg.username = username;
    mqtt_cfg.password = password;
    mqtt_cfg.keepalive = keepalive;
    mqtt_cfg.event_handle = mqttConnectionEventHandler;
    // mqtt_cfg.user_context = (void *)your_context;

    ESP_LOGI(TAG, "Connecting...\n");

    client = esp_mqtt_client_init(&mqtt_cfg);
}
void mqttConnectionWiFiConnected(void){
	mqttConnectionSetClient();
	esp_mqtt_client_start(client);
}

void mqttConnectionWiFiDisconnected(void){
	esp_mqtt_client_stop(client);
}

static void mqttConnectionTask(void *arg){

	while (true){

		message_t message;
		if (!xQueueReceive(mqttConnectionQueue, &message, 4000 / portTICK_RATE_MS)) {
			continue;
		}

		wifiUsed();

		EventBits_t eventBits = xEventGroupWaitBits(mqttConnectionEventGroup, MQTT_CONNECTED_BIT, false, true, 30000 / portTICK_RATE_MS);

		if (!(eventBits & MQTT_CONNECTED_BIT)) {
			ESP_LOGE(TAG, "Not connected after message received. Skipping message.");
			continue;
		}

		char mqttTopic[sizeof(message.deviceName) + sizeof(message.sensorName) + 1] = {0};
		char mqttValue[sizeof(message.stringValue)] = {0};

		strcpy(mqttTopic, message.deviceName);
		strcat(mqttTopic, "/");
		strcat(mqttTopic, message.sensorName);

		switch (message.valueType){

			case MESSAGE_INT:
				sprintf(mqttValue, "%d", message.intValue);
			break;

			case MESSAGE_FLOAT:
				sprintf(mqttValue, "%.4f", message.floatValue);
			break;

			case MESSAGE_DOUBLE:
				sprintf(mqttValue, "%.8f", message.doubleValue);
			break;

			case MESSAGE_STRING:
				sprintf(mqttValue, "%s", message.stringValue);
			break;
		}

		int msg_id;
		msg_id = esp_mqtt_client_publish(client, mqttTopic, mqttValue, 0, 1, 0);

		ESP_LOGI(TAG, "T: %s, V: %s -> MQTT %d\n", mqttTopic, mqttValue, msg_id);
		ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
	}
}

void mqttConnectionQueueAdd(message_t * message) {

	if (!uxQueueSpacesAvailable(mqttConnectionQueue)) {
		ESP_LOGE(TAG, "No room in queue for message.");
		return;
	}

	xQueueSend(mqttConnectionQueue, message, 0);
}

void mqttConnectionInit(void){

	mqttConnectionEventGroup = xEventGroupCreate();

	mqttConnectionQueue = xQueueCreate(4, sizeof(message_t));

	xTaskCreate(&mqttConnectionTask, "mqttConnection", 4096, NULL, 13, NULL);

	wifiUsed();
}

void mqttConnectionResetNVS(void) {
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttHost", "mqtt.server.example.com"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "mqttPort", 1883));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttUsername", "Username"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttPassword", "Password"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "mqttKeepalive", 30));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);

	messageNVSReset("mqtt", 0x00);
}