#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include "common.h"
#include "network.h"
#include "waterheater.h"
#include "haier_misc.h"
#include "haier_device.h"

#define HAIER_REMAIN_LENGTH(pointer, number)	\
	(sizeof(*pointer) - offsetof(typeof(*pointer), number) - sizeof(pointer->number))

static inline void
_haier_make_discover(haier_discover_t *discover)
{
	int length;
	assert(NULL != discover);
	memset(discover, 0, sizeof(*discover));
	memcpy(discover->magic, HAIER_MANUFACT_MAGIC, sizeof(discover->magic));
	discover->RSV1 = htonl(0x00006915);
	length = HAIER_REMAIN_LENGTH(discover, length);
	discover->length = htonl(length);
	strncpy(discover->version, HAIER_DISCOVER_VERSION, sizeof(discover->version));
	strncpy(discover->key, HAIER_DISCOVER_MAGIC, sizeof(discover->key));
}
/*********************************************/
typedef struct{
	char *ifname;
	int wait;
}haier_discover_request_t;
static
void *haier_discover_thread(void *param)
{
	int ret;
	uint32_t delay;
	uint32_t remain;
	int sock = -1;
	char broadcast[64];
	haier_discover_t discover;
	haier_discover_request_t *req = param;

	assert(NULL != param);
	pthread_detach(pthread_self());
	remain = req->wait * 1000;

	_haier_make_discover(&discover);
	//get broadcast address
	ret = network_get_broadcast(req->ifname, broadcast, sizeof(broadcast));
	if (ret)
		goto __end;
	sock = network_connect(broadcast, HAIER_WATERHEATER_DISCOVER_PORT,
				NETWORK_SOCK_BROADCAST);
	if (sock < 0)
		goto __end;

	delay = 100;
	while(remain > 0)
	{
		//send discover message every 1s
		ret = write(sock, (char*)&discover, sizeof(discover));
		if (unlikely(ret <= 0))
		{
			ERR("Send discover message fail: %d", ret);
			goto __end;
		}
		if (remain <= delay)
			delay = remain;
		usleep(delay * 1000);
		
		remain -= delay;
		delay <<= 1;
		if (delay > 1000)
			delay = 100;
	}//while(wait > 0)

__end:
	if (sock >= 0)
		close(sock);
	
	return NULL;
}

/*********************************************/
static inline int
_haier_check_discover_response(const haier_discover_resp_t *resp)
{
	int ret;
	uint32_t length;
	struct in_addr addr;
	assert(NULL != resp);
	//check manufacturer fist
	if (strncmp(resp->magic, HAIER_MANUFACT_MAGIC, sizeof(resp->magic)))
		return -EINVAL;
	if (strncmp(resp->key, HAIER_DISCOVER_RESPONSE_MAGIC, sizeof(resp->key)))
		return -EINVAL;
	length = HAIER_REMAIN_LENGTH(resp, length);
	if (resp->length != ntohs(length))
		return -EINVAL;
	ret = inet_aton(resp->ip, &addr);
	if (!ret)
		return -EINVAL;
		
	return 0;
}

/*********************************************/
int haier_discover(const char *ifname, const char *mac, int wait)
{
	int ret;
	char ip[65];
	int sock = -1;
	struct timeval tv;
	pthread_t pid;
	haier_discover_request_t *req = NULL;

	if (NULL == ifname)
		ifname = network_get_default_ifname();
	//get IP of interface
	if (NULL == network_get_ip(ifname, ip, sizeof(ip)))
	{
		ret = errno;
		ERR("Get IP from %s fail: %s", ifname, strerror(ret));
		goto __error_0;
	}

	//create sock for discover response
	sock = network_listen(ip, HAIER_WATERHEATER_DISCOVER_PORT, NETWORK_SOCK_UDP);
	if (unlikely(sock < 0))
	{
		ret = errno;
		ERR("Create discover response socket fail: %s", strerror(ret));
		goto __error_0;
	}
	
	//create thread for discover
	int len;
	int size;
	len = strlen(ifname);
	size = sizeof(*req) + len + 1;
	req = malloc(size);
	if (unlikely(NULL == req))
	{
		ret = ENOMEM;
		ERR("Alloc memory with size %d fail", size);
		goto __error_1;
	}
	req->ifname = (char *)(&req[1]);
	memcpy(req->ifname, ifname, len);
	req->ifname[len] = '\0';
	req->wait = wait;

	ret = pthread_create(&pid, NULL, haier_discover_thread, req);
	if (unlikely(ret))
	{
		ERR("Create thread for discover fail");
		goto __error_2;
	}

	//wait for response
	tv.tv_sec = wait;
	tv.tv_usec = 0;
	while(1)
	{
		fd_set rfds;
		haier_discover_resp_t resp;

		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		ret = select(sock+1, &rfds, NULL, NULL, &tv);
		if (0 == ret)	//timeout
		{
			ret = -1;
			INFO("Timeout");
			goto __error_3;
		}else if (ret < 0)
		{
			ERR("Socket error: %s", strerror(errno));
			goto __error_3;
		}
		//data ready
		ret = read(sock, (char*)&resp, sizeof(resp));
		if (ret != sizeof(resp))	//invalid response, ignore
			continue;
		ret = _haier_check_discover_response(&resp);
		if (ret)			//invalid response
			continue;

		//response ready
		//check if device already added
		haier_device_t *dev = haier_device_find(resp.MAC);
		if (NULL != dev)
			continue;
		dev = haier_device_add(resp.ip, resp.MAC);
		if (NULL == dev)
		{
			ret = ENOMEM;
			goto __error_3;
		}

		haier_print_discover_resp(&resp);
		//check if mac match
		if (NULL != mac)
		{
			if (!strcmp(mac, resp.MAC))	//mac match
				break;
		}
	}

	ret = 0;

__error_3:
	pthread_cancel(pid);
__error_2:
	free(req);
__error_1:
	close(sock);
__error_0:
	return ret;
}

/*********************************************/
int haier_discover_init(void)
{
	return 0;
}
void haier_discover_uninit(void)
{
}

