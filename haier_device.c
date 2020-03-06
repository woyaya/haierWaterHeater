#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "list.h"
#include "common.h"
#include "network.h"
#include "haier_device.h"
#include "haier_cmd.h"
#include "haier_misc.h"
#include "waterheater.h"

typedef enum{
	HAIER_DEVICE_HEATING,
	HAIER_DEVICE_KEEPING,
}haier_device_stat;
typedef enum{
	HAIER_DEVICE_POWER_OFF,
	HAIER_DEVICE_POWER_LOW,
	HAIER_DEVICE_POWER_MIDDLE,
	HAIER_DEVICE_POWER_HIGH,
}haier_device_power;

typedef struct{
	uint8_t hour;
	uint8_t minute;
	uint8_t temperature;
}haier_device_reserve_t;

typedef struct{
	uint16_t device_hour;
	uint16_t device_minute;

	uint16_t dest_temperature;
	uint16_t current_temperature;
	
	haier_device_reserve_t reserve[2];

	haier_device_power power;
	haier_device_stat stat;
}haier_device_status_t;



static LIST_HEAD(devices);
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static
haier_device_t *_haier_device_find(const char *mac)
{
	struct list_head *list;
	haier_device_t *dev;
	haier_device_t *dst = NULL;
	list_for_each(list, &devices)
	{
		dev = list_entry(list, haier_device_t, list);
		if (!strcmp(mac, dev->mac))
		{
			dst = dev;
			break;
		}
	}
	return dst;
}

static inline
haier_device_t *_haier_device_new(const char *ip, const char *mac)
{
	int ret;
	haier_device_t *dev;
	assert((NULL != ip) && (NULL != mac));
	dev = calloc(1, sizeof(*dev));
	if (NULL == dev)
	{
		ERR("Alloc memory for new device(%s) fail: %s", mac, strerror(errno));
		return NULL;
	}
	//INIT_LIST_HEAD(&dev->list);
	INIT_LIST_HEAD(&dev->list);
	dev->heartbeat = HAIER_HEARTBEAT_PERIOD;
	dev->cmd_serial = 1;
	dev->fd = -1;
	ret = strlen(ip);
	if (ret >= sizeof(dev->ip))
	{
		ERR("Invalid IP length: %s", ip);
		goto __error;
	}
	memcpy(dev->ip, ip, ret+1);
	ret = strlen(mac);
	if (ret >= sizeof(dev->mac))
	{
		ERR("Invalid MAC: %s", mac);
		goto __error;
	}
	memcpy(dev->mac, mac, ret+1);
	pthread_cond_init(&dev->cond, NULL);
	pthread_mutex_init(&dev->lock, NULL);
	dev->count = 0;
	HAIER_STAT_INIT(&dev->stat);
	INIT_LIST_HEAD(&dev->requests);

	return dev;

__error:
	free(dev);
	return NULL;
}

haier_device_t * haier_device_add(const char *ip, const char *mac)
{
	haier_device_t *dev;
	assert((NULL != ip) && (NULL != mac)); 
	pthread_mutex_lock(&lock);
	dev = _haier_device_find(mac);
	if (NULL == dev)
	{
		dev = _haier_device_new(ip, mac);
		if (NULL != dev)
			list_add_tail(&dev->list, &devices);
	}
	pthread_mutex_unlock(&lock);

	return dev;
}

int haier_device_del(haier_device_t *dev)
{
	pthread_mutex_lock(&lock);
	list_del(&dev->list);
	pthread_mutex_unlock(&lock);
	pthread_cond_broadcast(&dev->cond);
	usleep(100*1000);
	pthread_cond_destroy(&dev->cond);
	pthread_mutex_destroy(&dev->lock);

	free(dev);
	return 0;
}

haier_device_t *haier_device_find(const char *mac)
{
	haier_device_t *dev;
	pthread_mutex_lock(&lock);
	dev = _haier_device_find(mac);
	pthread_mutex_unlock(&lock);
	return dev;
}

/**********************************************/
int __haier_device_get_stat(haier_device_t *dev, Haier_stat_t *stat, int *count, int serial, int wait)
{
	int ret = -1;
	struct timespec timeout;
	haier_cmd_stat_t request;
	if (NULL == count)
		request.count = 0;
	else
		request.count = *count;
	request.serial = serial;
	haier_cmd_add_stat(dev, &request);

	if (0 == wait)
		wait = 2*60+1;
	
	{
		struct timeval now;
		gettimeofday(&now, NULL);
		timeout.tv_sec = now.tv_sec + wait;
		timeout.tv_nsec = now.tv_usec * 1000;
	}

	ret = 0;
	pthread_mutex_lock(&dev->lock);
	while(!haier_cmd_stat_check(dev, &request))
	{
		ret = pthread_cond_timedwait(&dev->cond, &dev->lock, &timeout);
		if (ret)
			break;
	}
	if (0 == ret)
	{
		memcpy(stat, &request.stat, sizeof(*stat));
		if (NULL != count)
			*count = request.count;
	}else
		HAIER_STAT_INIT(stat);
	pthread_mutex_unlock(&dev->lock);
	haier_cmd_del_stat(dev, &request);

	return ret;
}
/**********************************************/
int haier_device_get_stat(haier_device_t *dev, Haier_stat_t *stat, int *count, int wait)
{
	return __haier_device_get_stat(dev, stat, count, 0, wait);
}
/*return: 
	>= 0: command index
	<0: error
*/
typedef int (*prepare_t)(haier_device_t *dev, void *buffer, int *size, void *param);
static
int __haier_device_send(haier_device_t *dev, Haier_stat_t *stat, prepare_t func, void *param)
{
	int ret;
	int size;
	int serial;
	char data[256];
	size = sizeof(data);
	ret = func(dev, data, &size, param);
	if (unlikely(ret < 0))
	{
		ERR("Prepare cmd (%p(%p)) fail", func, param);
		return EINVAL;
	}
	serial = ret;
	ret = write(dev->fd, data, size);
	if (ret != size)
	{
		ERR("Write fail(%d!=%d): %s", ret, size, strerror(errno));
		return ret;
	}
	if (NULL == stat)
		return 0;
	//wait stat
	ret = __haier_device_get_stat(dev, stat, NULL, serial, 1);
	if (ret)//get stat fail
		HAIER_STAT_INIT(stat);

	return 0;
}

static inline
int haier_device_send_heartbeat(haier_device_t *dev, uint32_t serial)
{
	return __haier_device_send(dev, NULL, haier_cmd_prepare_heartbeat, NULL);
}
int haier_device_set_switch(haier_device_t *dev, Haier_stat_t *stat, int on)
{
	return __haier_device_send(dev, stat, haier_cmd_prepare_set_switch, (void *)(long)on);
}
int haier_device_set_temperature(haier_device_t *dev, Haier_stat_t *stat, uint32_t temp)
{
	return __haier_device_send(dev, stat, haier_cmd_prepare_set_temp, (void *)(unsigned long)temp);
}
int haier_device_set_mode(haier_device_t *dev, Haier_stat_t *stat, int reserve)
{
	return __haier_device_send(dev, stat, haier_cmd_prepare_set_mode, (void *)(long)reserve);
}
/**********************************************/
static
void *haier_device_thread(void *param)
{
	int ret;
	uint32_t serial = 0;
	int errors = 0;
	struct timeval timeout;
	haier_device_t *dev = (haier_device_t*)param;
	assert(NULL != dev);
	pthread_detach(pthread_self());

	timeout.tv_sec= 0;
	timeout.tv_usec = 0;

	while(1)
	{
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(dev->fd, &readfds);
		if ((0 == timeout.tv_sec) && (0 == timeout.tv_usec))
			timeout.tv_sec = dev->heartbeat;
		//wait for next event happen
		ret = select(dev->fd+1, &readfds, NULL, NULL, &timeout);
		if (unlikely(ret < 0))
		{//some error happen
			ret = errno;
			if (EBADF == ret)
			{
				ERR("Select return EBADF");
				break;
			}
			if (EINTR == ret)
				continue;
			continue;
		}else if (0 == ret)
		{//timeout, send heartbeat
			haier_device_send_heartbeat(dev, serial++);
			continue;
		}
		if (FD_ISSET(dev->fd, &readfds))
		{//new data ready
			char data[256];
			ret = read(dev->fd, data, sizeof(data));
			if (unlikely(ret <= 0))
			{
				ret = errno;
				if (EBADF == ret)
					break;
				errors++;
				continue;
			}
			haier_cmd_process(dev, data, ret);
		}
	}

	//delete device
	//haier_device_delete(dev->mac);

	return NULL;
}

int haier_device_uninit(haier_device_t *dev)
{
	assert(NULL != dev);
	if (dev->fd >= 0)
	{
		int ret = dev->fd;
		dev->fd = -1;
		close(ret);
	}
	pthread_cancel(dev->pid);
	return 0;
}

/*param: retry: 0: always retry 
		>0: try count = 'retry'
 */
int haier_device_init(haier_device_t *dev, int retry)
{
	int ret;
	assert(NULL != dev);
	//connect to device
	do{
		dev->fd = network_connect(dev->ip, HAIER_WATERHEATER_COMMUNIC_PORT, NETWORK_SOCK_TCP);
		if (dev->fd >= 0)
			break;
		//delay befor retry
		if (1 != retry)
			sleep(1);
	}while(--retry);
	if (dev->fd < 0)
	{
		//maybe it powerdown
		ERR("Connect to remote %s:%d fail", dev->ip, HAIER_WATERHEATER_COMMUNIC_PORT);
		return ENODEV;
	}
	//create thread for device
	ret = pthread_create(&dev->pid, NULL, haier_device_thread, dev);
	if (unlikely(ret))
	{
		ERR("Create thread for %s(%s) fail", dev->ip, dev->mac);
		close(dev->fd);
		dev->fd = -1;
	}

	return ret;
}



