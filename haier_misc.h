#ifndef __HAIER_MISC_H__
#define __HAIER_MISC_H__

#include "waterheater.h"

void haier_heartbeat_h2n(Haier_heartbeat_t *heartbeat);
void haier_heartbeatack_n2l(Haier_heartbeat_ack_t *ack);
void haier_header_h2n(Haier_header_t *header);
void haier_header_n2h(Haier_header_t *header);
void haier_cmd_h2n(Haier_cmd_t *cmd);
void haier_cmd_n2h(Haier_cmd_t *cmd);
void haier_stat_n2h(Haier_stat_t *stat);

/*********************************************/
uint8_t haier_checksum_calculate(const uint8_t *buffer, int len);
/*********************************************/
void haier_print_discover_resp(const haier_discover_resp_t *resp);
void haier_print_heartbeat_ack(const Haier_heartbeat_ack_t *ack);
void haier_print_header(const Haier_header_t *head);
void haier_print_cmd(const Haier_cmd_t *cmd);
/*********************************************/
int haier_discoveresp2json(const haier_discover_resp_t *resp, char *buf, int size);
int haier_stat2json(const Haier_stat_t *stat, char *buf, int size);


#endif	//__HAIER_MISC_H__

