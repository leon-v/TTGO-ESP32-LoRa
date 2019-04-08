#ifndef __RADIO_H_
#define __RADIO_H_

void radioInit(void);
xQueueHandle radioGetQueue(void);
void radioResetNVS(void);

#endif