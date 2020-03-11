#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "waterheater.h"

#include <arpa/inet.h>
void haier_heartbeat_h2n(Haier_heartbeat_t *heartbeat)
{
	heartbeat->type = htonl(heartbeat->type);
	heartbeat->serial = htonl(heartbeat->serial);
	heartbeat->remain = htonl(heartbeat->remain);
}
void haier_heartbeatack_n2l(Haier_heartbeat_ack_t *ack)
{
	ack->type = ntohl(ack->type);
	ack->serial = ntohl(ack->serial);
	ack->remain = ntohl(ack->remain);
	ack->val = ntohl(ack->val);
}

void haier_header_h2n(Haier_header_t *header)
{
	header->type = htonl(header->type);
	header->serial = htonl(header->serial);
	header->remain = htonl(header->remain);
}
void haier_header_n2h(Haier_header_t *header)
{
	header->type = ntohl(header->type);
	header->serial = ntohl(header->serial);
	header->remain = ntohl(header->remain);
}

void haier_cmd_h2n(Haier_cmd_t *cmd)
{

}
void haier_cmd_n2h(Haier_cmd_t *cmd)
{

}

void haier_stat_n2h(Haier_stat_t *stat)
{
	stat->current_temp = ntohs(stat->current_temp);
	stat->dest_temp = ntohs(stat->dest_temp);
	stat->hour = ntohs(stat->hour);
	stat->minute = ntohs(stat->minute);
}

/*********************************************/
void haier_mac_format(char *mac)
{
	int i = 0;
	int offset = 0;
	assert(NULL != mac);
	while('\0' != mac[i])
	{
		if ((mac[i] >= 'a') && (mac[i] <= 'z'))
			mac[offset++] = mac[i] - ('a' - 'A');
		else if (((mac[i] >= '0') && (mac[i] <= '9')) || ((mac[i] >= 'A') && (mac[i] <= 'Z')))
			mac[offset++] = mac[i];
		else{
			//skip invalid data
		}
		i++;
	}
	mac[offset] = '\0';
	
	return;
}
/*********************************************/
uint8_t haier_checksum_calculate(const uint8_t *buffer, int len)
{
	int i;
	uint32_t checksum = 0;
	assert(NULL != buffer);
	for (i=0;i<len;++i)
		checksum += *buffer++;
	return checksum & 0xff;
}

/*********************************************/
int haier_discoveresp2json(const haier_discover_resp_t *resp, char *buf, int size)
{
	int ret;
	assert((NULL != resp) && (NULL != buf));
	ret = snprintf(buf, size,
		"{"
		"\n\t\"Magic\":\"%s\","
		"\n\t\"MAC\":\"%s\","
		"\n\t\"Nickname\":\"%s\","
		"\n\t\"SSID\":\"%s\","
		"\n\t\"IP\":\"%s\","
		"\n\t\"Version\":\"%s,%s,%.8s,%.8s\","
		"\n\t\"Magic\":\"%s\""
		"\n}"
		,
		resp->magic,
		resp->MAC,
		resp->nickname,
		resp->ssid,
		resp->ip,
		resp->version1, resp->version2, resp->version3, resp->version4,
		resp->key);
	assert(ret < size);
	return ret;
}

void WEAK_SYMBOL haier_print_discover_resp(const haier_discover_resp_t *resp)
{
	char buf[256];
	haier_discoveresp2json(resp, buf, sizeof(buf));
	INFO("\nDEVICE\n%s\n", buf);
}

void haier_print_heartbeat_ack(const Haier_heartbeat_ack_t *ack)
{
	assert(NULL != ack);
	INFO("Heartbeat ack %d: %s", ack->serial, ack->MAC);
}

void haier_print_header(const Haier_header_t *head)
{
	const char *type;
	assert(NULL != head);
	if (HAIER_WATERHEAT_CMD_CONTROL == head->type)
		type = "Control";
	else
		type = "Response";
	INFO("%s header:\n"
		"\tMAC: %s\n"
		"\tSerial: %d\n"
		"\tRemain length: %d",
		type,
		head->MAC,
		head->serial,
		head->remain);
}

void haier_print_cmd(const Haier_cmd_t *cmd)
{
	assert(NULL != cmd);
	ERR("Command: length: %d, type: 0x%02x",
		cmd->length, cmd->type);
}
/*********************************************/
int haier_stat2json(const Haier_stat_t *stat, char *buf, int size)
{
	int ret;
	int power = 0;
	const char *onoff;
	const char *heating;
	const char *dynamic;

	assert((NULL != stat) && (NULL != buf));
	if (HAIER_STAT_INVALID(stat))
	{
		buf[0] = '\0';
		ERR("Invalid stat");
		return 0;
	}

	power += stat->status & HAIER_STAT_STATUS_1200W ? 1200 : 0;
	power += stat->status & HAIER_STAT_STATUS_800W ? 800 : 0;
	onoff = stat->status & HAIER_STAT_STATUS_ON ? "on" : "off";
	heating = stat->status2 & HAIER_STAT_STATUS_HEATING ? "heating" : "insulation";
	dynamic = stat->status2 & HAIER_STAT_STATUS_DYNAMIC ? "dynamic" : "normal";

	if (stat->status & HAIER_STAT_STATUS_RESERVE)
	{
		ret = snprintf(buf, size,
			"{"
			"\n\t\"Switch\":\"%s\","
			"\n\t\"Reserve\":\"on\","
			"\n\t\"Power\":%d,"
			"\n\t\"Stat\":\"%s\","
			"\n\t\"Dynamic\":\"%s\","
			"\n\t\"CurrentTemperature\":%d,"
			"\n\t\"DeviceTime\":\"%02d:%02d\","
			"\n\t\"Reserve1\":{"
			"\n\t\t\"Temperature\":%d,"
			"\n\t\t\"Time\":\"%02d:%02d\","
			"\n\t\t\"Stat\":\"%s\""
			"\n\t},"
			"\n\t\"Reserve2\":{"
			"\n\t\t\"Temperature\":%d,"
			"\n\t\t\"Time\":\"%02d:%02d\","
			"\n\t\t\"Stat\":\"%s\""
			"\n\t}"
			"\n}"
			,
			onoff, power,
			heating,
			dynamic,
			stat->current_temp,
			stat->hour, stat->minute,
			stat->temp_1, stat->hour_1 & HAIER_STAT_HOUR_MASK, stat->minute_1, stat->hour_1 & HAIER_STAT_HOUR_ENABLE ? "disable" : "enable",
			stat->temp_2, stat->hour_2 & HAIER_STAT_HOUR_MASK, stat->minute_2, stat->hour_2 & HAIER_STAT_HOUR_ENABLE ? "disable" : "enable");


	}else
	{
		ret = snprintf(buf, size,
			"{"
			"\n\t\"Switch\":\"%s\","
			"\n\t\"Reserve\":\"off\","
			"\n\t\"Power\":%d,"
			"\n\t\"Stat\":\"%s\","
			"\n\t\"Dynamic\":\"%s\","
			"\n\t\"CurrentTemperature\":%d,"
			"\n\t\"DistTemperature\":%d,"
			"\n\t\"DeviceTime\":\"%02d:%02d\""
			"\n}"
			,
			onoff, power,
			heating,
			dynamic,
			stat->current_temp,
			stat->dest_temp,
			stat->hour, stat->minute
			);
	}
	assert(ret < size);
	return ret;
}
