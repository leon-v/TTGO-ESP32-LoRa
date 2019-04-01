#ifndef __MESSAGE_QUEUE_H_
#define __MESSAGE_QUEUE_H_

#define MESSAGE_INT 0
#define MESSAGE_FLOAT 1
#define MESSAGE_DOUBLE 2
#define MESSAGE_STRING 3

typedef struct{
	char deviceName[16];
	char sensorName[16];
	int valueType;
	int intValue;
	float floatValue;
	double doubleValue;
	char stringValue[32];
} message_t;

#endif