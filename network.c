#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
//#include <linux/ethtool.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#define _GNU_SOURCE
#include <string.h>
#include <limits.h>

#include "common.h"
#include "network.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef DEFAULT_IFNAME
  #define DEFAULT_IFNAME	"br-lan"
//#define DEFAULT_IFNAME	"zt0"
#endif

static inline 
int __network_ioctl(const char *ifname, int request, struct ifreq *ifr)
{
	int ret;
	int sock;
	assert((NULL != ifname) && (NULL != ifr));
	if (-1 == (sock=socket(AF_INET, SOCK_DGRAM, 0)))
	{
		ERR("Create socket error:%s", strerror(errno));
		goto __error;
	}
	strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
	ifr->ifr_name[sizeof(ifr->ifr_name) - 1] = '\0';
	ret = ioctl(sock, request, ifr);
	close(sock);
	if (ret < 0)
	{
		printf("%s: Ioctl error: %s\n", ifname, strerror(errno));
		goto __error;
	}
	return 0;
__error:
	return -errno;
}

const char *network_get_default_ifname(void)
{
	return DEFAULT_IFNAME;
}

int network_get_mac(const char *ifname, char *buf, int size)
{
	int ret;
	struct ifreq ifr;
	assert((NULL != buf) && (size >= IFHWADDRLEN));

	ifname = NULL == ifname ? DEFAULT_IFNAME : ifname;

	ret = __network_ioctl(ifname, SIOCGIFHWADDR, &ifr);
	if (ret)
	{
		buf[0] = '\0';
		printf("Get MAC address of %s fail: %s\n", ifname, strerror(errno));
		return ret;
	}
	memcpy(buf, ifr.ifr_addr.sa_data, IFHWADDRLEN);

	return 0;
}

char *network_get_ip(const char *ifname, char *local_ip, int len)
{
	int ret;
	struct sockaddr_in sin;
	struct ifreq ifr;
	assert((NULL != local_ip) && (len >= 16));

	ifname = NULL == ifname ? DEFAULT_IFNAME : ifname;

	ret = __network_ioctl(ifname, SIOCGIFADDR, &ifr);
	if (ret)
		goto __error;
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	snprintf(local_ip, len, "%s", inet_ntoa(sin.sin_addr));
	local_ip[len-1] = '\0';
//	DBG("Get local IP as %s", local_ip);
	return local_ip;
__error:
	local_ip[0] = '\0';
	return local_ip;
}

int network_get_broadcast(const char *ifname, char *ip, int len)
{
	int ret;
	struct sockaddr_in sin;
	struct ifreq ifr;
	assert((NULL != ip) && (len >= 16));

	ifname = NULL == ifname ? DEFAULT_IFNAME : ifname;

	ret = __network_ioctl(ifname, SIOCGIFBRDADDR, &ifr);
	if (ret)
		goto __error;
	memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
	snprintf(ip, len, "%s", inet_ntoa(sin.sin_addr));
	ip[len-1] = '\0';
//	DBG("Get local IP as %s", local_ip);
	return 0;
__error:
	ip[0] = '\0';
	return ret;
}


/***************************************/
int network_connect(const char *ip, int port, network_sock_type proto)
{
	int ret;
	int sock = -1;
	int type;
	int protocol;
	int broadcase = 0;
	struct in_addr addr;
	struct sockaddr_in sin;
	assert((NULL != ip) && (port > 0));
	switch(proto)
	{
		case NETWORK_SOCK_TCP:
			type = SOCK_STREAM;
			protocol = IPPROTO_TCP;
		break;
		case NETWORK_SOCK_BROADCAST:
			broadcase = 1;
		//go cross
		case NETWORK_SOCK_UDP:
			type = SOCK_DGRAM;
			protocol = IPPROTO_UDP;
		break;
		default:
			ERR("Invalid sock type: %d", proto);
			errno = EINVAL;
			goto __error;
		break;
	}
	sock = socket(AF_INET, type, protocol);
	if (sock < 0)
	{
		ERR("Create socket error");
		goto __error;
	}
	if (broadcase)
	{
		broadcase = 1;
		setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
				&broadcase, sizeof(broadcase));
	}
	ret = inet_aton(ip, &addr);
	if (!ret)
	{
		ERR("Invalid IP: %s", ip);
		errno = -EINVAL;
		goto __error;
	}
	//connect
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip);
	sin.sin_port = htons(port);
	if (connect(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in)))
	{
		ERR("connect remote target fail: %s", strerror(errno));
		goto __error;
	}
	return sock;
__error:
	if (sock >= 0)
		close(sock);
	return -1;
}
int network_listen(const char *ip, int port, network_sock_type proto)
{
	int sock = -1;
	int type;
	int protocol;
	struct sockaddr_in serv_addr;
	if (NULL == ip)
		ip = "0.0.0.0";
	assert(port > 0);
	switch(proto)
	{
		case NETWORK_SOCK_TCP:
			type = SOCK_STREAM;
			protocol = IPPROTO_TCP;
		break;
		case NETWORK_SOCK_UDP:
			type = SOCK_DGRAM;
			protocol = IPPROTO_UDP;
		break;
		default:
			ERR("Invalid sock type: %d", proto);
			errno = EINVAL;
			goto __error;
		break;
	}
	sock = socket(AF_INET, type, protocol);
	if (sock < 0)
	{
		ERR("Create socket error");
		goto __error;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)))
	{
		ERR("bind local ip&port fail: %s", strerror(errno));
		goto __error;
	}	

	return sock;

__error:
	if (sock >= 0)
		close(sock);
	return -1;
}
/***************************************/
int network_mac_delete_colon(char *mac_dst, int size, const char *mac_src)
{
	int i;
	int len;
	assert((NULL != mac_dst) && (NULL != mac_src));
	assert(size >= IFHWADDRLEN*2+1);
	len = strlen(mac_src);
	if (len != IFHWADDRLEN * 3 - 1)
	{
		ERR("Invalid MAC length: %d", len);
		return EINVAL;
	}
	
	for (i=0;i<IFHWADDRLEN;i++)
	{
		mac_dst[i*2] = mac_src[i*3];
		mac_dst[i*2+1] = mac_src[i*3+1];
		if ((':' == mac_src[i*3+2]) || ('\0' == mac_src[i*3+2]))
			continue;
		//invalid char
		ERR("Invalid char(%c) in %s", mac_src[i*3+2], mac_src);
		return EINVAL;
	}
	mac_dst[i*2] = '\0';
	return 0;
}
int network_mac2string(const uint8_t *mac, int mac_len, char *str, int size)
{
	int i;
	assert((NULL != mac) && (mac_len >= IFHWADDRLEN));
	assert((NULL != str) && (size >= (IFHWADDRLEN*3)));

	for(i=0;i<IFHWADDRLEN;i++)
	{
		sprintf(str+i*3, "%02X:", mac[i]);
	}
	str[i*3-1] = '\0';

	return 0;
}

static
int hex_digit( char ch )
{
	if(('0' <= ch) && (ch <= '9')) ch -= '0';
	else if (('a' <= ch) && (ch <= 'f')) ch += 10 - 'a';
	else if (('A' <= ch) && (ch <= 'F')) ch += 10 - 'A';
	else
		return -1;
	return ch;
}
int network_string2mac(const char *str, int str_len, uint8_t *mac, int size)
{
	int i;
	int ret;
	char buf[IFHWADDRLEN*2+1];
	assert((NULL != mac) && (size >= IFHWADDRLEN));
	assert((NULL != str)/* && (str_len >= (IFHWADDRLEN*3))*/);
	ret = strlen(str);
	if (IFHWADDRLEN*3-1 == ret)
	{
		ret = network_mac_delete_colon(buf, sizeof(buf), str);
		if (unlikely(ret))
			goto __error;
	}else if (IFHWADDRLEN*2 == ret)
		memcpy(buf, str, ret+1);
	else
		goto __error;
	for (i=0;i<IFHWADDRLEN;++i)
	{
		int result[2];
		result[0] = hex_digit(buf[2 * i]);
		result[1] = hex_digit(buf[2 * i+ 1]);
		if ((-1 == result[0]) || (-1 == result[1]))
			goto __error;

		mac[i] = (result[0] << 4) | result[1];
	}
	return 0;
	
__error:
	ERR("Invalid mac string: %s", str);
	return EINVAL;
}

