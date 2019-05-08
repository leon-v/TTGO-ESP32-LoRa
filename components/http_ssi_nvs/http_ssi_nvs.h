#ifndef HTTP_SSI_NVS_H_
#define HTTP_SSI_NVS_H_

void httpSSINVSGet(httpd_req_t *req, char * ssiTag);
void httpSSINVSSet(char * ssiTag, char * value);

#endif