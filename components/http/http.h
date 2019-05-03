#ifndef HTTP_SERVER_H

#include <esp_http_server.h>

typedef struct {
	const char * uri;
	const char * page;
	const char * type;
} httpPage_t;

typedef enum {
	SSI_TYPE_TEXT = 0,
	SSI_TYPE_DISAPLY_TEXT,
	SSI_TYPE_TEXTAREA,
    SSI_TYPE_PASSWORD,
    SSI_TYPE_INTEGER,
    SSI_TYPE_CHECKBOX
} ssiTagType_t;

typedef struct{
	const char * key;
	const ssiTagType_t type;
} ssiTag_t;

typedef struct{
	char * key;
	char * value;
} token_t;

typedef struct{
	token_t tokens[32];
	unsigned int length;
} tokens_t;

void httpServerInit(void);
esp_err_t httpRespond(httpd_req_t *req, const char * fileStart, const char * fileEnd, const ssiTag_t * ssiTags, int ssiTagsLength);
void httpPageRegister(httpd_handle_t server, const httpPage_t * httpPage);

#define MAX_HTTP_SSI_KEY_LENGTH 32

#define HTTP_SERVER_H
#endif