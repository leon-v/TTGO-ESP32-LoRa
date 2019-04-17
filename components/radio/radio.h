#ifndef __RADIO_H_
#define __RADIO_H_

#include "message.h"

void radioLoRaQueueAdd(message_t * message);
void radioInit(void);
void radioResetNVS(void);


#endif