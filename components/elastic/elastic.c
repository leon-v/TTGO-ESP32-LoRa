#include <nvs.h>
#include <esp_log.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "wifi.h"
#include "esp_http_client.h"
#include "datetime.h"
#include "cJSON.h"
#include "elastic.h"

#define TAG "ElasticHTTP"

#define MAX_CONFIG_STRING_LENGTH 128

static xQueueHandle elasticMessageQueue = NULL;

xQueueHandle getSlasticMessageQueue(void){
	return elasticMessageQueue;
}

esp_err_t elasticHTTPEventHandler(esp_http_client_event_t *evt) {

	int httpResponseCode = 0;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            // ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            // ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            // printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:

			httpResponseCode = esp_http_client_get_status_code(evt->client);

            // ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, code=%d, len=%d", httpResponseCode, evt->data_len);

            if ( (httpResponseCode < 200) || (httpResponseCode > 299) ) {
            	ESP_LOGE(TAG, "%.*s", evt->data_len, (char*)evt->data);
			}


            // if (!esp_http_client_is_chunked_response(evt->client)) {

            	// printf("%.*s", evt->data_len, (char*)evt->data);

//             	cJSON * response = NULL;
//             	cJSON * shards = NULL;
//             	cJSON * successful = NULL;

// 				response = cJSON_Parse((char*) evt->data);

// 				if (response == NULL) {
// 					ESP_LOGI(TAG, "Error parsing JSON");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				shards = cJSON_GetObjectItemCaseSensitive(response, "_shards");

// 				if (shards == NULL) {
// 					ESP_LOGI(TAG, "Error parsing JSON, Cant find _shards");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				successful = cJSON_GetObjectItemCaseSensitive(shards, "successful");

// 				if (successful == NULL) {
// 					ESP_LOGI(TAG, "Error parsing JSON, Cant find successful");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				if (!cJSON_IsNumber(successful)) {
// 					ESP_LOGI(TAG, "Error parsing JSON, Successful not number");
// 					goto EVENT_DATA_CLEANUP;
// 				}

// 				if (successful->valueint <= 0){
// 					ESP_LOGI(TAG, "ERROR: %d shards were created in elastic.", successful->valueint);
// 				}else{break;
// 					ESP_LOGI(TAG, "%d shards were created in elastic.", successful->valueint);
// 				}

// EVENT_DATA_CLEANUP:

// 				if (shards) {
// 					cJSON_Delete(shards);
// 				}

// 				if (successful) {
// 					cJSON_Delete(successful);
// 				}

// 				if (response) {
// 					cJSON_Delete(response);
// 				}
            // }

            break;
        case HTTP_EVENT_ON_FINISH:
            // ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            // ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}


static void elasticTask(void *arg){

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READONLY, &nvsHandle));

    size_t nvsLength;

    char url[MAX_CONFIG_STRING_LENGTH + 16];
    nvsLength = sizeof(url);
	nvs_get_str(nvsHandle, "elasticURL", url, &nvsLength);

	unsigned int port;
	nvs_get_u32(nvsHandle, "elasticPort", &port);

	char uniqueName[16];
	nvsLength = sizeof(uniqueName);
	nvs_get_str(nvsHandle, "uniqueName", uniqueName, &nvsLength);

	strcat(url, "/beeline_");
	strcat(url, uniqueName);
	strcat(url, "/router");


	nvs_close(nvsHandle);

	struct tm timeinfo;

	esp_err_t err;

	while (1){

		vTaskDelay(2); // Delay for 2 ticks to allow scheduler time to execute

		EventBits_t eventBits = xEventGroupWaitBits(wifiGetEventGroup(), WIFI_CONNECTED_BIT | TIME_READY_BIT, false, true, 5000 / portTICK_RATE_MS);

		if (!(eventBits & WIFI_CONNECTED_BIT)) {
			continue;
		}

		if (!(eventBits & TIME_READY_BIT)) {
			continue;
		}

		elasticMessage_t elasticMessage;
		// elasticMessage_t

		if (xQueueReceive(elasticMessageQueue, &elasticMessage, 1000 / portTICK_RATE_MS)) {

			cJSON * request;
			request = cJSON_CreateObject();

			char valueString[16] = {0};
			sprintf(valueString, "%.2f", elasticMessage.value);

			cJSON_AddItemToObject(request, elasticMessage.name, cJSON_CreateRaw(valueString));

			cJSON_AddItemToObject(request, "name", cJSON_CreateString(elasticMessage.name));

			cJSON_AddItemToObject(request, "value", cJSON_CreateRaw(valueString));

			cJSON_AddItemToObject(request, "deviceName", cJSON_CreateString(uniqueName));

			localtime_r(&elasticMessage.time, &timeinfo);

			char timestamp[64] = "";
    		strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &timeinfo);
			cJSON_AddItemToObject(request, "timestamp", cJSON_CreateString(timestamp));

			char * postData = cJSON_Print(request);

			esp_http_client_config_t config = {
				.url = url,
				.event_handler = elasticHTTPEventHandler,
				.auth_type = HTTP_AUTH_TYPE_BASIC,
				.method = HTTP_METHOD_POST,
			};

			esp_http_client_handle_t client = esp_http_client_init(&config);

			err = esp_http_client_set_method(client, HTTP_METHOD_POST);

			err = esp_http_client_set_post_field(client, postData, strlen(postData));

			err = esp_http_client_set_header(client, "Content-Type", "application/json");

			err = esp_http_client_perform(client);

			if (err == ESP_OK) {

				int httpResponseCode = 0;
				httpResponseCode = esp_http_client_get_status_code(client);

				// ESP_LOGI(TAG, "N: %s, V: %.2f, D: %s -> Elastic. HTTP Status %d, Response Length: %d.\n",
				// 	elasticMessage.name,
				// 	elasticMessage.value,
				// 	timestamp,
				// 	httpResponseCode,
    //             	esp_http_client_get_content_length(client)
				// );
			}

			esp_http_client_cleanup(client);

			free(postData); // Why not free in cJSON_Delete !?
			cJSON_Delete(request);
		}
	}


}

void elasticInit(void) {

	elasticMessageQueue = xQueueCreate(4, sizeof(elasticMessage_t));

	xTaskCreate(&elasticTask, "elasticTask", 8192, NULL, 13, NULL);
}

void elasticResetNVS(void) {

	nvs_handle nvsHandle;
	ESP_ERROR_CHECK(nvs_open("BeelineNVS", NVS_READWRITE, &nvsHandle));

	ESP_ERROR_CHECK(nvs_set_str(nvsHandle, "elasticURL", "http://username:password@elastic.example.com:9200/beeline/router"));
	ESP_ERROR_CHECK(nvs_commit(nvsHandle));

	nvs_close(nvsHandle);
}