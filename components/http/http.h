#ifndef HTTP_SERVER_H

#include <esp_http_server.h>

typedef struct {
	const char * uri;
	const char * page;
	const char * type;
} httpPage_t;

typedef struct {
	const char * uri;
	const char * start;
	const char * end;
	const char * type;
} httpFile_t;

typedef struct{
	char * key;
	char * value;
} token_t;

typedef struct{
	token_t tokens[32];
	unsigned int length;
} tokens_t;

void httpServerInit(void);
void httpPageRegister(httpd_handle_t server, const httpPage_t * httpPage);
void httpFileRegister(httpd_handle_t server, const httpFile_t * httpFile);

#define MAX_HTTP_SSI_KEY_LENGTH 32

#define HTTP_SERVER_H
#endif