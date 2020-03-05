#ifndef _HAIER_DEVICE_H__
#define _HAIER_DEVICE_H__
#include <stdint.h>
#include <pthread.h>

#include "list.h"
#include "waterheater.h"

typedef struct{
	struct list_head list;	//device list
	char mac[16];
	char ip[4*4];

	uint32_t heartbeat;	//heartbeat period
	uint32_t heartbeat_serial;	//heartbeat serial number

	int32_t cmd_serial;	//command serail number

	int fd;
	pthread_t pid;

	pthread_cond_t cond;
	pthread_mutex_t lock;
	volatile int count;	//received stat count
	Haier_stat_t stat;
	struct list_head requests;	//stat request list
}haier_device_t;

void haier_device_print(const haier_device_t *device);

haier_device_t *haier_device_find(const char *mac);
int haier_device_del(haier_device_t *dev);
haier_device_t *haier_device_add(const char *ip, const char *mac);
/**********************************************/
int haier_device_set_switch(haier_device_t *dev, Haier_stat_t *stat, int on);
int haier_device_set_mode(haier_device_t *dev, Haier_stat_t *stat, int reserve);
int haier_device_set_temperature(haier_device_t *dev, Haier_stat_t *stat, uint32_t temp);
int haier_device_get_stat(haier_device_t *dev, Haier_stat_t *stat, int *index, int wait);
/**********************************************/
int haier_device_uninit(haier_device_t *dev);
/*param: retry: 0: always retry 
		>0: try count = 'retry'
 */
int haier_device_init(haier_device_t *dev, int retry);
#endif //_HAIER_DEVICE_H__

