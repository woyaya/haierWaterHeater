#ifndef __NETWORK_H_
#define __NETWORK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
const char *network_get_default_ifname(void);

extern
int network_get_mac(const char *ifname, char *buf, int size);
extern
char *network_get_ip(const char *ifname, char *local_ip, int len);

extern
int network_get_broadcast(const char *ifname, char *ip, int len);
/******************************************************************/
typedef enum{
	NETWORK_SOCK_TCP,
	NETWORK_SOCK_UDP,
	NETWORK_SOCK_BROADCAST,
}network_sock_type;
int network_connect(const char *ip, int port, network_sock_type type);
int network_listen(const char *ip, int port, network_sock_type proto);
/******************************************************************/
int network_mac_delete_colon(char *mac_dst, int size, const char *mac_src);
int network_mac2string(const uint8_t *mac, int mac_len, char *str, int size);
int network_string2mac(const char *str, int str_len, uint8_t *mac, int size);
#ifdef __cplusplus
}
#endif

#endif

