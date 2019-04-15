#ifndef __RADIO_H_
#define __RADIO_H_

#include "message.h"

void radioInit(void);
void radioResetNVS(void);
void radioLoRaQueueAdd(message_t * message);

#endif