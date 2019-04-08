#ifndef __MESSAGE_QUEUE_H_
#define __MESSAGE_QUEUE_H_

enum messageType_t {
	MESSAGE_INT,
	MESSAGE_FLOAT,
	MESSAGE_DOUBLE,
	MESSAGE_STRING,
};

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