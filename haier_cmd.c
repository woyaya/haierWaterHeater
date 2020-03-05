#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "list.h"
#include "common.h"
#include "network.h"
#include "haier_device.h"
#include "haier_cmd.h"
#include "haier_misc.h"
#include "waterheater.h"
/**********************************************/
int haier_cmd_add_stat(haier_device_t *dev, haier_cmd_stat_t *stat)
{
	assert((NULL != dev) && (NULL != stat));
	pthread_mutex_lock(&dev->lock);
	list_add_tail(&stat->list, &dev->requests);
	pthread_mutex_unlock(&dev->lock);
	return 0;
}
int haier_cmd_del_stat(haier_device_t *dev, haier_cmd_stat_t *stat)
{
	assert((NULL != dev) && (NULL != stat));
	pthread_mutex_lock(&dev->lock);
	list_del_init(&stat->list);
	pthread_mutex_unlock(&dev->lock);
	return 0;
}
//haier_device_t::lock should be locked befor call this function
//return 0: not ready; others: ready
int haier_cmd_stat_check(haier_device_t *dev, haier_cmd_stat_t *stat)
{
	assert((NULL != dev) && (NULL != stat));
	if (0 == stat->serial)
	{
		if (dev->count != stat->count)
		{
			stat->count = dev->count;
			memcpy(&stat->stat, &dev->stat, sizeof(stat->stat));
			return 1;
		}
	}else
	{
		if (list_empty(&stat->list))
			return 1;
	}
	return 0;
}
//haier_device_t::lock should be locked befor call this function
static inline
int haier_cmd_stat_ready(haier_device_t *dev, int serial, const Haier_stat_t *stat)
{
	struct list_head *pos;
	struct list_head *next;
	haier_cmd_stat_t *request;
	assert((NULL != dev) && (NULL != stat));
	dev->count++;
	memcpy(&dev->stat, stat, sizeof(dev->stat));
	if (0 == serial)
		return 0;
	list_for_each_safe(pos, next, &dev->requests)
	{
		request = list_entry(pos, haier_cmd_stat_t, list);
		if (serial != request->serial)
			continue;
		request->count = dev->count;
		memcpy(&request->stat, stat, sizeof(request->stat));
		list_del_init(&request->list);
	}
	return 0;
}
/**********************************************/
Haier_cmd_t *haier_cmd_find(uint8_t *data, int length)
{
	int i;
	if (length <= 2)
		return NULL;
	length -= 1;
	for (i=0;i<length;++i)
	{
		if (0xFF != *data++)
			continue;
		if (0xFF == *data)
		{
			if (length-i < sizeof(Haier_cmd_t))
				return NULL;
			return (Haier_cmd_t*)(data+1);
		}
	}
	return NULL;
}
int haier_cmd_check(const Haier_cmd_t *cmd)
{
	uint8_t data;
	uint8_t checksum = haier_checksum_calculate((uint8_t*)cmd, cmd->length);
	if (cmd->length <= sizeof(*cmd))
	{
		ERR("Invalid command size: %d <= %d", cmd->length, sizeof(*cmd));
		return EINVAL;
	}
	data = cmd->data[cmd->length - sizeof(*cmd)];
	if (checksum != data)
	{
		ERR("Checksum unmatch: 0x%x != 0x%x", checksum, data);
		return EINVAL;
	}
	return 0;
}

static inline
int __haier_cmd_process(haier_device_t *dev, const Haier_header_t *header, const Haier_cmd_t *cmd)
{
	int len = header->remain;
	len -= 3;
	if (cmd->length > len)
	{
		ERR("Invalid command length: %d <= %d", cmd->length, len);
		goto __error;
	}
	if (haier_cmd_check(cmd))
	{
		ERR("Invalid command");
		goto __error;
	}

	switch(cmd->type)
	{
		case HAIER_CMD_TYPE_STAT:
		case HAIER_CMD_TYPE_RESP_STAT:
		{
			Haier_stat_t *stat = (Haier_stat_t *)cmd->data;
			haier_stat_n2h(stat);
			pthread_mutex_lock(&dev->lock);
			haier_cmd_stat_ready(dev, header->serial, stat);
			pthread_mutex_unlock(&dev->lock);
			pthread_cond_broadcast(&dev->cond);
		}
		break;
		default:
			ERR("Unsupport command: 0x%x", cmd->type);
			goto __error;
		break;
	}

	return 0;

__error:
	haier_print_cmd(cmd);
	return EINVAL;
}

int haier_cmd_process(haier_device_t *dev, void *data, int size)
{
	int ret = EINVAL;
	uint32_t type;
	if (size <= sizeof(type))
	{
		ERR("Invalid size: %d", size);
		goto __error;
	}
	memcpy(&type, data, sizeof(type));
	type = ntohl(type);
	switch(type)
	{
		case HAIER_WATERHEAT_CMD_HEARTBEAT_RESPONSE:
		{
			Haier_heartbeat_ack_t *ack = (Haier_heartbeat_ack_t*)data;
			haier_heartbeatack_n2l(ack);
			//haier_print_heartbeat_ack(ack);
		}
		break;
		case HAIER_WATERHEAT_CMD_CONTROL_RESPONSE:
		{
			Haier_header_t *header = (Haier_header_t *)data;
			haier_header_n2h(header);
			size -= sizeof(*header);
			if (size < header->remain)
			{
				ERR("Invalid size: %d < %d", 
					size, header->remain);
				haier_print_header(header);
				goto __error;
			}

			dev->cmd_serial = header->serial + 1;
			if (dev->cmd_serial <= 0)
				dev->cmd_serial = 1;

			Haier_cmd_t* cmd;
			cmd = haier_cmd_find(header->data, header->remain);
			if (NULL == cmd)
			{
				ERR("Invalid command");
				haier_print_header(header);
				goto __error;
			}
			haier_cmd_n2h(cmd);
			__haier_cmd_process(dev, header, cmd);
		}
		break;
		default:
			MSG("Unknow command: 0x%x", type);
			goto __error;
		break;
	}

	ret = 0;
__error:
	return ret;
}


/**********************************************/
static const
Haier_heartbeat_t haier_heartbeat_init = HAIER_HEARTBEAT_INITIAL;
int haier_cmd_prepare_heartbeat(haier_device_t *dev, void *data, int *size, void *param)
{
	Haier_heartbeat_t *hb;
	assert(*size >= sizeof(Haier_heartbeat_t));
	assert((NULL != dev) && (NULL != data));
	hb = (Haier_heartbeat_t *)data;
	*hb = haier_heartbeat_init;
	hb->serial = dev->heartbeat_serial++;
	strncpy(hb->MAC, dev->mac, sizeof(hb->MAC)-1);
	hb->MAC[sizeof(hb->MAC)-1] = '\0';
	haier_heartbeat_h2n(hb);
	*size = sizeof(*hb);
	return 0;
}

static
int haier_cmd_prepare_header(haier_device_t *dev, Haier_header_t *header, uint32_t type, int *size)
{
	int32_t serial = dev->cmd_serial;
	Haier_cmd_t *cmd = (Haier_cmd_t *)(header->data+2);
	*size = 2 + cmd->length + 1;

	header->type = type;
	memcpy(header->MAC, dev->mac, sizeof(header->MAC));
	header->serial = serial;
	header->remain = *size;
	haier_header_h2n(header);
	header->data[0] = 0xff;
	header->data[1] = 0xff;

	*size += sizeof(*header);
	return serial;
}
static
void haier_cmd_prepare_cmd(Haier_cmd_t *cmd, int type, int size)
{
	int total = size + sizeof(*cmd);
	memset(cmd, 0, sizeof(*cmd));
	cmd->length = total;
	cmd->type = type;
	cmd->data[size] = haier_checksum_calculate((uint8_t*)cmd, total);
	haier_cmd_h2n(cmd);
}

#if 0
int haier_cmd_prepare_getstat(haier_device_t *dev, void *data, int size, void *param)
{
	Haier_cmd_t *cmd;
	Haier_header_t *header;

	assert((NULL != dev) && (NULL != data));
	assert(size >= (sizeof(*header)+2/*0xFFFF*/+sizeof(*cmd)+1/*checksum*/));
	memset(data, 0, size);
	header = (Haier_header_t *)data;
	cmd = (Haier_cmd_t *)(header->data+2);

	haier_cmd_prepare_cmd(cmd, 0x09, 0);
	return haier_cmd_prepare_header(dev, header, HAIER_WATERHEAT_CMD_CONTROL);
}
#endif

static
int _haier_cmd_prepare_control(haier_device_t *dev, void *buffer, int *size, uint32_t type, uint32_t data)
{
	Haier_cmd_t *cmd;
	Haier_header_t *header;
	Haier_control_t *control;

	assert((NULL != dev) && (NULL != buffer));
	assert(*size >= (sizeof(*header)+2/*0xFFFF*/+sizeof(*cmd)+sizeof(*control)+1/*checksum*/));
	memset(buffer, 0, *size);
	header = (Haier_header_t *)buffer;
	cmd = (Haier_cmd_t *)(header->data+2);
	control = (Haier_control_t *)cmd->data;

	control->cmd = type;
	control->cmd2 = 0x5d;
	control->data = data;

	haier_cmd_prepare_cmd(cmd, 0x01, sizeof(*control));
	return haier_cmd_prepare_header(dev, header, HAIER_WATERHEAT_CMD_CONTROL, size);
}
int haier_cmd_prepare_set_temp(haier_device_t *dev, void *buffer, int *size, void *param)
{
	uint32_t temperature = (uint32_t)param;
	if ((temperature < HAIER_TEMPERATURE_MIN) || (temperature > HAIER_TEMPERATURE_MAX))
	{
		ERR("Dist temperature out of range[%d,%d]: %d", 
			HAIER_TEMPERATURE_MIN, HAIER_TEMPERATURE_MAX, temperature);
		return EINVAL;
	}
	return _haier_cmd_prepare_control(dev, buffer, size, HAIER_TEMPERATURE, temperature);
}
int haier_cmd_prepare_set_mode(haier_device_t *dev, void *buffer, int *size, void *param)
{
	int data = ((uint32_t)param) ? 1 : 0;
	return _haier_cmd_prepare_control(dev, buffer, size, HAIER_RESERVE, data);
}
int haier_cmd_prepare_set_switch(haier_device_t *dev, void *buffer, int *size, void *param)
{
	int data = ((uint32_t)param) ? 1 : 0;
	return _haier_cmd_prepare_control(dev, buffer, size, HAIER_SWITCH, data);
}
