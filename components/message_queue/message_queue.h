#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

typedef enum {
	float,
	double,
	int,
	string
} messageQueueValueType_t;

typedef struct {
	messageQueueValueType_t valueType : 8;
	size_t valueBytes : 8;

} messageQueueMessahe_t;

#endif