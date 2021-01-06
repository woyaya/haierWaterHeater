#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#define _GNU_SOURCE
#include <string.h>

#include "common.h"
#include "network.h"
#include "haier_discover.h"
#include "haier_device.h"
#include "haier_cmd.h"
#include "haier_misc.h"

static const char *ip = NULL;
static const char *mac = NULL;
static const char *ifname = NULL;
static int wait = 1;
static int state = ~0;
static int temperature = -1;
static int reserve = -1;
static uint8_t onoff = -1;
static uint8_t json = 0;
static uint8_t discover = 0;
unsigned short log_level = LOG_LEVEL_DEBUG;
/**********************************************/
void haier_print_discover_resp(const haier_discover_resp_t *resp)
{
	int ret;
	char buf[256];
	if (!discover)
		return;
	ret = haier_discoveresp2json(resp, buf, sizeof(buf));
	if (ret > 0)
	{
		if (json)
			printf("%s\n\n", buf);
		else
			printf("DEVICE\n%s\n\n", buf);
	}
}
static
void haier_print_stat(const Haier_stat_t *stat)
{
	int ret;
	char buf[512];
	ret = haier_stat2json(stat, buf, sizeof(buf));
	if (ret > 0)
	{
		if (json)
			printf("%s\n\n", buf);
		else
			printf("STAT\n%s\n\n", buf);
	}
}


int haier_get_stat(haier_device_t *dev, int count, int delay)
{
	int ret;
	int period = 0;
	int index = 0;
	int remain = count ? count : 1;
	Haier_stat_t state;
	struct timeval timeout;
	gettimeofday(&timeout, NULL);
	timeout.tv_sec += delay;
	while(remain)
	{
		if (0 == delay) 
			period = HAIER_STATREPORT_PERIOD + 1;
		else
		{
			struct timeval now;
			gettimeofday(&now, NULL);
			if (now.tv_sec < timeout.tv_sec)
			{
				period = timeout.tv_sec - now.tv_sec;
				if (period > HAIER_STATREPORT_PERIOD + 1)
					period = HAIER_STATREPORT_PERIOD + 1;
			}else if ((now.tv_sec > timeout.tv_sec) || (now.tv_usec > timeout.tv_usec))
			{
				ret = ETIMEDOUT;
				break;
			}else
				period = 1;
		}

		ret = haier_device_get_stat(dev, &state, &index, period);
		if (0 == ret)
		{
			haier_print_stat(&state);
			if (0 != count) remain -= 1;
			continue;
		}
		if (ETIMEDOUT == ret)
			continue;
		ERR("Get stat fail: %d", ret);
		break;
	}

	return ret;
}
/**********************************************/

static
void usage(const char *name)
{
	assert(NULL != name);
	printf("Haier smart water heater(EC6002-D6) controler\n"
		"Usage:\n"
		"%s [OPTs] [MAC_address]\n"
		"\t-I <ifname>: search devices on network interface\n"
		"\t-i <ip>: IP address of device\n"
		"\t-m <mac>: Mac address. If not specified, search all device in the LAN\n"
		"\t-s <second>: search device and wait. 0: wait forever. Default: 1\n"
		"\t-g <count>: get water heater stat 'count' times. 0: endless get\n"
		"\t-T <temperature>: set dist temperature. valid range: [%d:%d]\n"
		"\t-o 0|1: set switch to on/off. 0: switch off; 1: switch on\n"
		"\t-r 0|1: set reserve mode. 0: direct mode; 1: reserve mode\n"
		"\t-j: print result as json\n"
		"\t-R: print result of search device. Default: don't print\n"
		, name, HAIER_TEMPERATURE_MIN, HAIER_TEMPERATURE_MAX);
}

static 
int __param_parse(int argc, char * const argv[])
{
	int c;
	long result;

	if (argc <= 1)
		goto __error;
	//Scan the command line for options
	while (-1 != (c = getopt (argc, argv, "I:i:m:s:g:T:o:r:jR")))
	{
		switch(c)
		{
			case 'I':	//ethernet name
				ifname = optarg;
			break;
			case 'i':
				ip = optarg;
			break;
			case 'm':
				haier_mac_format(optarg);
				mac = optarg;
			break;
			case 's':
				result = strtol(optarg, NULL, 10);
				if (result < 0)
				{
					ERR("Invalid wait time: %s", optarg);
					goto __error;
				}
				wait = result;
			break;
			case 'g':
				result = strtol(optarg, NULL, 10);
				if (result < 0)
				{
					ERR("Invalid get count: %s", optarg);
					goto __error;
				}
				state = result;
			break;
			case 'T':
				result = strtol(optarg, NULL, 10);
				if ((result < HAIER_TEMPERATURE_MIN) || (result > HAIER_TEMPERATURE_MAX))
				{
					ERR("Temperature out of range[%d:%d]:%ld",
						HAIER_TEMPERATURE_MIN, HAIER_TEMPERATURE_MAX,
						result);
					goto __error;
				}
				temperature = result;
			break;
			case 'o':
			case 'r':
				if (('\0'!=optarg[1]) || (('0'!=optarg[0])&&('1'!=optarg[0])))
				{
					ERR("Invalid param: %s", optarg);
					goto __error;
				}
				if ('0' == optarg[0])
				{
					if ('o' == c)
						onoff = 0;
					if ('r' == c)
						reserve = 0;
				}else
				{
					if ('o' == c)
						onoff = 1;
					if ('r' == c)
						reserve = 1;
				}
			break;
			case 'j':
				json = 1;
			break;
			case 'R':
				discover = 1;
			break;
			default:
				ERR("Unkonw option: %c", (char)c);
				goto __error;
			break;
		}
	}

	return 0;
__error:
	usage(argv[0]);
	return -1;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	if (__param_parse(argc, argv))
		return -1;
	ret = haier_discover_init();
	if (unlikely(ret))
	{
		ERR("Discover initial fail");
		return ret;
	}

	if (NULL == ip)
	{//no IP, search first
		ret = haier_discover(ifname, mac, wait);
	}else
	{
		if (NULL == mac)
		{
			ERR("Missing device mac");
			ret = EINVAL;
			goto __end;
		}
		haier_device_add(ip, mac);
	}
	if (NULL == mac)
		goto __end;
	haier_device_t *dev;
	dev = haier_device_find(mac);
	if (unlikely(NULL == dev))
	{
		ERR("Can not find device: %s", mac);
		ret = ENODEV;
		goto __end;
	}

	ret = haier_device_init(dev, wait);
	if (unlikely(ret))
	{
		ERR("Initial device (%s:%s) fail", dev->ip, dev->mac);
		goto __end;
	}

	ret = 0;
	if (state >= 0)	//get stat
	{
		if (0 == state) wait = 0;
		haier_get_stat(dev, state, wait);
	}else
	{
		Haier_stat_t stat;
		if ((0 == onoff) || (1 == onoff))
			ret = haier_device_set_switch(dev, &stat, onoff);
		if ((0 == reserve) || (1 == reserve))
			ret = haier_device_set_mode(dev, &stat, reserve);
		if (temperature > 0)
			ret = haier_device_set_temperature(dev, &stat, temperature);
		if (0 == ret)	//success
		{
			haier_print_stat(&stat);
		}
	}
	//Do not execut command too fast
	sleep(1);

	haier_device_uninit(dev);

__end:
	haier_discover_uninit();
	return ret;
}	
