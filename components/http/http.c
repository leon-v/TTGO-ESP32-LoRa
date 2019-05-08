#include <sys/param.h>
#include <esp_http_server.h>
#include <http_parser.h>
#include <esp_log.h>

#include "http.h"
#include "http_ssi_nvs.h"
#include "http_ssi_functions.h"
#include "http_ssi_page.h"

#include "wifi.h"

#include "config_device_html.h"
#include "config_diesensors_html.h"
#include "config_display_html.h"
#include "config_elasticsearch_html.h"
#include "config_lora_html.h"
#include "config_mqtt_html.h"
#include "config_ntp_html.h"
#include "config_wifi_html.h"
#include "favicon_png.h"
#include "index_html.h"
#include "javascript_js.h"
#include "menu_css.h"
#include "menu_html.h"
#include "runtime_stats.h"
#include "style_css.h"

#define TAG "http"

#define HTTP_STACK_SIZE 16384
#define HTTP_BUFFER_SIZE HTTP_STACK_SIZE / 2

#define START_SSI "<!--#"
#define END_SSI "-->"

static void httpServerURLDecode(char * input, int length) {

    char * output = input;
    char hex[3] = "\0\0\0";

    while (input[0] != '\0') {

    	if (!length--){
    		break;
    	}


    	if (input[0] == '+') {
    		output[0] = ' ';
    		input+= 1;
    	}

    	else if (input[0] == '%') {

    		hex[0] = input[1];
    		hex[1] = input[2];
    		output[0] = strtol(hex, NULL, 16);
    		input+= 3;
    	}

    	else{
    		output[0] = input[0];
    		input+= 1;
    	}

    	output+= 1;
    }

    output[0] = '\0';
}

static char * httpServerParseValues(tokens_t * tokens, char * buffer, const char * rowDelimiter, const char * valueDelimiter, const char * endMatch){

	tokens->length = 0;

	// Start parsing the values by creating a new string from the payload
	char * token = strtok(buffer, rowDelimiter);

	char * end = buffer + strlen(buffer);

	// break apart the string getting all the parts delimited by &
	while (token != NULL) {

		if (strlen(endMatch) > 0){
			end = token + strlen(token) + 1;

			if (strncmp(end, endMatch, strlen(endMatch)) == 0) {
				end+= strlen(endMatch);
				break;
			}
		}

		tokens->tokens[tokens->length++].key = token;


		token = strtok(NULL, rowDelimiter);
	}

	// Re-parse the strigns and break them apart into key / value pairs
	for (unsigned int index = 0; index < tokens->length; index++){

		tokens->tokens[index].key = strtok(tokens->tokens[index].key, valueDelimiter);

		tokens->tokens[index].value = strtok(NULL, valueDelimiter);

		// If the value is NULL, make it point to an empty string.
		if (tokens->tokens[index].value == NULL){
			tokens->tokens[index].value = tokens->tokens[index].key + strlen(tokens->tokens[index].key);
		}

		httpServerURLDecode(tokens->tokens[index].key, CONFIG_HTTP_NVS_MAX_STRING_LENGTH);
		httpServerURLDecode(tokens->tokens[index].value, CONFIG_HTTP_NVS_MAX_STRING_LENGTH);
	}

	return end;
}


static char * httpServerGetTokenValue(tokens_t * tokens, const char * key){

	for (unsigned int index = 0; index < tokens->length; index++){

		if (strcmp(tokens->tokens[index].key, key) == 0){
			return tokens->tokens[index].value;
		}
	}

	return NULL;
}

static esp_err_t httpGetPost(httpd_req_t *req, char * postString, unsigned int postStringLength){

	int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, postString, MIN(remaining, postStringLength))) < 0) {
            return ESP_FAIL;
        }

        remaining -= ret;
    }

    postString[req->content_len] = '\0';

    return ESP_OK;
}

void stop_webserver(httpd_handle_t server) {
    // Stop the httpd server
    httpd_stop(server);
}

void httpPageReplaceTag(char * outBuffer, char * tag) {

	char * module = strtok(tag, ":");

	if (!module){
		strcpy(outBuffer, "Failed to get module from tag");
	}

	char * ssiTag = strtok(NULL, "");

	if (strcmp(module, "nvs") == 0){
		httpSSINVSGet(outBuffer, ssiTag);
	}

	else if (strcmp(module, "functions") == 0){
		httpSSIFunctionsGet(outBuffer, ssiTag);
	}

	else if (strcmp(module, "page") == 0){
		httpSSIPageGet(outBuffer, ssiTag);
	}

	else{
		strcpy(outBuffer, "Failed to parse module for tag");
	}
}
static void httpPageGetContent(char * outBuffer, httpd_req_t *req){

	httpPage_t * httpPage = (httpPage_t *) req->user_ctx;

	char tag[CONFIG_HTTP_NVS_MAX_STRING_LENGTH];

	char * end = httpPage->page;
	char * start = strstr(end, START_SSI);

	int length;

	if (!start){
		strcpy(outBuffer, end);
		return;
	}

	strcpy(outBuffer, "");


	while (start != NULL) {

		length = start - end;
		strncat(outBuffer, end, length);

		start+= strlen(START_SSI);

		end = strstr(end, END_SSI);

		if (!end){
			start = strstr(start, START_SSI);
			continue;
		}

		int length = end - start;

		memcpy(tag, start, length);
		tag[length] = '\0';

		char * outBufferEnd = outBuffer + strlen(outBuffer);

		httpPageReplaceTag(outBufferEnd, tag);

		end+= strlen(END_SSI);
		start = strstr(end, START_SSI);
	}

	strcat(outBuffer, end);
}

void httpPagePost(httpd_req_t *req, char * buffer, size_t bufferLength){

	ESP_ERROR_CHECK(httpGetPost(req, buffer, bufferLength));

	tokens_t post;
	httpServerParseValues(&post, buffer, "&", "=", "\0");

	for (int tokenIndex = 0; tokenIndex < post.length; tokenIndex++){

		token_t * token = &post.tokens[tokenIndex];

		char * module = strtok(token->key, ":");

		if (!module){
			ESP_LOGE(TAG, "Failed to get module from tag");
			continue;
		}

		if (strcmp(module, "nvs") == 0){

			char * ssiTag = strtok(NULL, "");
			httpSSINVSSet(ssiTag, token->value);

		}
		else{
			ESP_LOGE(TAG, "Failed to parse module for tag");
		}
	}
}

static esp_err_t httpPageHandler(httpd_req_t *req){

	httpPage_t * httpPage = (httpPage_t *) req->user_ctx;

	wifiUsed();

	char outBuffer[HTTP_BUFFER_SIZE];

	if (req->method == HTTP_POST) {
		httpPagePost(req, outBuffer, sizeof(outBuffer));
	}

	if (httpPage->type){
		httpd_resp_set_type(req, httpPage->type);
	}

	if (httpPage->page){
		httpPageGetContent(outBuffer, req);
	}
	else{
		strcpy(outBuffer, "Nothing found to populate content.");
	}

	return httpd_resp_send(req, outBuffer, strlen(outBuffer));
}

void httpPageRegister(httpd_handle_t server, const httpPage_t * httpPage){

	httpd_uri_t getURI = {
	    .uri      	= httpPage->uri,
	    .method   	= HTTP_GET,
	    .handler  	= httpPageHandler,
	    .user_ctx	= httpPage,
	};

	httpd_register_uri_handler(server, &getURI);

	httpd_uri_t postURI = {
	    .uri      = httpPage->uri,
	    .method   = HTTP_POST,
	    .handler  = httpPageHandler,
	    .user_ctx	= httpPage,
	};

	httpd_register_uri_handler(server, &postURI);
}

static esp_err_t httpFileHandler(httpd_req_t *req){

	httpFile_t * httpFile = (httpFile_t *) req->user_ctx;

	wifiUsed();

	if (httpFile->type){
		httpd_resp_set_type(req, httpFile->type);
	}

	size_t length = httpFile->end - httpFile->start;
	return httpd_resp_send(req, httpFile->start, length);
}

void httpFileRegister(httpd_handle_t server, const httpFile_t * httpFile){

	httpd_uri_t getURI = {
	    .uri      	= httpFile->uri,
	    .method   	= HTTP_GET,
	    .handler  	= httpFileHandler,
	    .user_ctx	= httpFile,
	};

	httpd_register_uri_handler(server, &getURI);
}

static httpd_handle_t start_webserver(void) {

	wifiUsed();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = HTTP_STACK_SIZE;

    config.max_uri_handlers = 32;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    httpd_handle_t server;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");

        httpPageConfigDeviceHTMLInit(server);
        httpPageConfigDieSensorsHTMLInit(server);
        httpPageConfigDisplayHTMLInit(server);
        httpPageConfigElasticsearchHTMLInit(server);
        httpPageConfigLoRaHTMLInit(server);
        httpPageConfigMQTTHTMLInit(server);
        httpPageConfigNTPHTMLInit(server);
        httpPageConfigWiFiHTMLInit(server);
        httpFileFaviconPNGInit(server);
        httpPageIndexHTMLInit(server);
        httpPageJavascriptJSInit(server);
        httpPageMenuCSSInit(server);
        httpPageMenuHTMLInit(server);
        httpPageRuntimeStatsInit(server);
        httpPageStyleCSSInit(server);

        return server;
    }

    printf("http: / Error starting server!");
    return NULL;
}

static void httpServerTask(void *arg){

	EventBits_t EventBits;
	httpd_handle_t server = NULL;

	while (true){

		vTaskDelay(2);

		EventBits = xEventGroupWaitBits(wifiGetEventGroup(), WIFI_CONNECTED_BIT, false, true, 500 / portTICK_RATE_MS);

		if (WIFI_CONNECTED_BIT & EventBits) {

			/* Start the web server */
	        if (server == NULL) {
	            server = start_webserver();
	        }
		}

		else{
			/* Stop the webserver */
	        if (server) {
	            stop_webserver(server);
	            server = NULL;
	        }
		}
	}

	vTaskDelete(NULL);
    return;
}

void httpServerInit(){
	xTaskCreate(&httpServerTask, "http", HTTP_STACK_SIZE, NULL, 14, NULL);
}
