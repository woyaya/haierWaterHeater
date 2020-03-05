#ifndef _HAIER_DISCOVER_H__
#define _HAIER_DISCOVER_H__

int haier_discover(const char *ifname, const char *mac, int wait);
int haier_discover_init(void);
void haier_discover_uninit(void);

#endif //_HAIER_DISCOVER_H__

