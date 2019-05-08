#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <nvs.h>
#include <mqtt_client.h>
#include <esp_log.h>
#include <sys/param.h>

#include "wifi.h"
// #include "http.h"
#include "mqtt_connection.h"
#include "message.h"

static esp_mqtt_client_handle_t client;
static esp_mqtt_client_config_t mqtt_cfg;

static xQueueHandle mqttConnectionQueue = NULL;

static EventGroupHandle_t mqttConnectionEventGroup;

static const char *TAG = "MQTT";

static const char * ROUTE_NAME = "mqtt";

static esp_err_t mqttConnectionEventHandler(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
        	xEventGroupSetBits(mqttConnectionEventGroup, MQTT_CONNECTED_BIT);
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            size_t nvsLength;
            nvs_handle nvsHandle;
			ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

			char mqttInTopic[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
			nvsLength = sizeof(mqttInTopic);
			nvs_get_str(nvsHandle, "mqttInTopic", mqttInTopic, &nvsLength);

			nvs_close(nvsHandle);

            msg_id = esp_mqtt_client_subscribe(client, mqttInTopic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

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



        	char topic[64] = {0};
        	strncpy(topic, event->topic,  MIN(event->topic_len, sizeof(topic)));

        	char value[64] = {0};
        	strncpy(value, event->data,  MIN(event->data_len, sizeof(value)));

        	// ESP_LOGI(TAG, "topic %s", topic);

        	const char * delim = "/";
        	char * subTopic = strtok(topic, delim);

        	char * subTopics[3] = {NULL, NULL, NULL};

        	int index = 0;
        	while (subTopic != NULL){

        		if (index > 1) {
        			subTopics[0] = subTopics[1];
        		}
        		if (index > 0) {
        			subTopics[1] = subTopics[2];
        		}

        		subTopics[2] = subTopic;

        		index++;

        		subTopic = strtok(NULL, delim);
        	}

        	if (!(subTopics[0] && subTopics[1] && subTopics[2])) {
        		ESP_LOGE(TAG, "Unable to get last 3 sub topics device/sensor/valueype");
        		break;
        	}

        	message_t message;

        	strcpy(message.deviceName, subTopics[0]);
        	strcpy(message.sensorName, subTopics[1]);

        	if (strcmp(subTopics[2], "int") == 0) {
				message.valueType = MESSAGE_INT;
				message.intValue = atoi(value);
			}
			else if (strcmp(subTopics[2], "float") == 0) {
				message.valueType = MESSAGE_FLOAT;
				message.floatValue = atof(value);
			}
			else if (strcmp(subTopics[2], "double") == 0) {
				message.valueType = MESSAGE_DOUBLE;
				message.doubleValue = atof(value);
			}
			else if (strcmp(subTopics[2], "string") == 0) {
				message.valueType = MESSAGE_STRING;
				strcpy(message.stringValue, value);
			}
			else{
				ESP_LOGE(TAG, "Unknown type '%s'", subTopics[2]);
        		break;
			}

			messageIn(&message, ROUTE_NAME);
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

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

	size_t nvsLength;

	char mqttOutTopic[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];
	nvsLength = sizeof(mqttOutTopic);
	nvs_get_str(nvsHandle, "mqttOutTopic", mqttOutTopic, &nvsLength);

	nvs_close(nvsHandle);

	while (true){

		message_t message;
		if (!xQueueReceive(mqttConnectionQueue, &message, 500 / portTICK_RATE_MS)) {
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

		strcpy(mqttTopic, mqttOutTopic);
		strcat(mqttTopic, "/");
		strcat(mqttTopic, message.deviceName);
		strcat(mqttTopic, "/");
		strcat(mqttTopic, message.sensorName);

		switch (message.valueType){

			case MESSAGE_INT:
				sprintf(mqttValue, "%d", message.intValue);
				strcat(mqttTopic, "/int");
			break;

			case MESSAGE_FLOAT:
				sprintf(mqttValue, "%.4f", message.floatValue);
				strcat(mqttTopic, "/float");
			break;

			case MESSAGE_DOUBLE:
				sprintf(mqttValue, "%.8f", message.doubleValue);
				strcat(mqttTopic, "/double");
			break;

			case MESSAGE_STRING:
				sprintf(mqttValue, "%s", message.stringValue);
				strcat(mqttTopic, "/string");
			break;
		}

		int msg_id;
		msg_id = esp_mqtt_client_publish(client, mqttTopic, mqttValue, 0, 1, 0);

		// ESP_LOGI(TAG, "T: %s, V: %s -> MQTT %d\n", mqttTopic, mqttValue, msg_id);
		// ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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

	xTaskCreate(&mqttConnectionTask, "mqttConnection", 4096, NULL, 10, NULL);

	wifiUsed();
}

void mqttConnectionResetNVS(void) {
	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttHost", "mqtt.server.example.com"));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "mqttPort", 1883));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttUsername", "Username"));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttPassword", "Password"));

	ESP_ERROR_CHECK(nvs_set_u32(nvsHandle, "mqttKeepalive", 30));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttOutTopic", "/beeline/in"));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "mqttInTopic", "/beeline/out/#"));

	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);

	messageNVSReset("mqtt", 0x00);
}