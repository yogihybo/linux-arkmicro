#ifndef __EVENT_H
#define __EVENT_H
#endif
#include<string.h>
#include<stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define event_handle event_t*

typedef struct  
{
	bool state;
	bool manual_reset;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}event_t;

#ifdef __cplusplus
extern "C" {
#endif

event_handle event_create(bool manual_reset, bool init_state);

int event_timedwait(event_handle hevent, long milliseconds);

int event_set(event_handle hevent);

int event_reset(event_handle hevent);

void event_destroy(event_handle hevent);

#ifdef __cplusplus
};
#endif

