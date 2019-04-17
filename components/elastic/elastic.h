#ifndef ELASTIC_H_
#define ELASTIC_H_

#include "message.h"

void elasticInit(void);
void elasticResetNVS(void);
void elasticQueueAdd(message_t * message);

#endif