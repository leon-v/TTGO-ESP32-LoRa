#ifndef ELASTIC_H_
#define ELASTIC_H_

void elasticInit(void);
void elasticResetNVS(void);
xQueueHandle getSlasticMessageQueue(void);

typedef struct{
	char name[32];
	float value;
	time_t time;
} elasticMessage_t;

#endif