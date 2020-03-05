#ifndef _HAIER_CMD_H__
#define _HAIER_CMD_H__
#include <stdint.h>
#include <pthread.h>

#include "list.h"
#include "waterheater.h"

typedef struct{
	struct list_head list;
	int count;	//valid if haier_cmd_stat_t::serial is 0
	int serial;
	Haier_stat_t stat;
}haier_cmd_stat_t;
/**********************************************/
int haier_cmd_add_stat(haier_device_t *dev, haier_cmd_stat_t *stat);
int haier_cmd_del_stat(haier_device_t *dev, haier_cmd_stat_t *stat);
int haier_cmd_stat_check(haier_device_t *dev, haier_cmd_stat_t *stat);
//haier_device_t::lock should be locked befor call this function
//return 0: not ready; others: ready
int haier_cmd_check_stat(haier_cmd_stat_t *stat);
/**********************************************/
Haier_cmd_t *haier_cmd_find(uint8_t *data, int length);
int haier_cmd_check(const Haier_cmd_t *cmd);
int haier_cmd_process(haier_device_t *dev, void *data, int len);
/**********************************************/
int haier_cmd_prepare_heartbeat(haier_device_t *dev, void *data, int *size, void *param);
int haier_cmd_prepare_set_temp(haier_device_t *dev, void *buffer, int *size, void *param);
int haier_cmd_prepare_set_mode(haier_device_t *dev, void *buffer, int *size, void *param);
int haier_cmd_prepare_set_switch(haier_device_t *dev, void *buffer, int *size, void *param);
#endif //_HAIER_CMD_H__

