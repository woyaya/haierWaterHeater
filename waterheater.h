#ifndef __HAIER_WATERHEATER_H__
#define __HAIER_WATERHEATER_H__

#include <stdint.h>
#define HAIER_WATERHEATER_DISCOVER_PORT		7083
#define HAIER_WATERHEATER_COMMUNIC_PORT		56800

//0000   48 61 69 65 72 00 00 69 15 00 00 00 00 00 00 00  Haier..i........
//0010   00 00 00 00 38 00 00 00 00 00 00 00 00 00 00 00  ....8...........
//0020   00 00 00 00 00 32 2e 30 2e 30 00 00 00 55 44 49  .....2.0.0...UDI
//0030   53 43 4f 56 45 52 59 5f 53 44 4b 00 00 00 00 00  SCOVERY_SDK.....
//0040   00 00 00 00 00 00 00 00 00 00 00 00 00           .............
typedef struct{			//port: UDP:7083
	char magic[5];		//Haier
#define HAIER_MANUFACT_MAGIC		"Haier"
	uint32_t RSV1;		//0x00006915
	char RSV2[10];		//0x00
	uint16_t length;	//remain length

	char RSV3[16];		//0x00
	char version[8];	//version: 2.0.0
#define HAIER_DISCOVER_VERSION	"2.0.0"
	char key[32];		//UDISCOVERY_SDK
#define HAIER_DISCOVER_MAGIC	"UDISCOVERY_SDK"
}__attribute__ ((packed))haier_discover_t;

//0000   48 61 69 65 72 00 00 68 4d 00 00 00 00 00 00 00  Haier..hM.......
//0010   00 00 00 01 20 30 30 30 37 41 38 xx xx xx xx xx  .... 0007A8xxxxx
//0020   xx 00 00 00 00 11 1c 12 00 24 00 08 10 06 01 00  x........$......
//0030   41 80 03 32 00 00 00 00 00 00 00 00 00 00 00 00  A..2............
//0040   00 00 00 00 00 6e 65 77 20 6e 69 63 6b 6e 61 6d  .....new nicknam
//0050   65 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  e...............
//0060   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0070   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0080   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0090   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//00a0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//00b0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//00c0   00 00 00 00 00 6b 69 6e xx xx xx xx xx xx xx 67  .....kinxxxxxx4g
//00d0   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//00e0   00 00 00 00 00 31 39 32 2e 31 36 38 2e 30 2e 32  .....192.168.0.2
//00f0   30 33 00 00 00 30 2e 30 2e 30 00 00 00 32 2e 31  03...0.0.0...2.1
//0100   35 00 00 00 00 65 5f 31 2e 33 2e 30 30 47 5f 31  5....e_1.3.00G_1
//0110   2e 30 2e 30 30 55 44 49 53 43 4f 56 45 52 59 5f  .0.00UDISCOVERY_
//0120   55 57 54 00 00 00 00 00 00 00 00 00 00 00 00 00  UWT.............
//0130   00 00 00 00 00                                   .....
typedef struct{			//port: UDP:7083
	char magic[5];		//Haier
	
	uint32_t RSV1;		//0x0000684d
	char zero1[10];		//0x00
	uint16_t length;	//remain length

	char MAC[16];		//MAC address
	uint8_t RSV4[16];	//11 1c 12 00 24 00 08 10 06 01 00 41 80 03 32 00
	uint8_t zero2[16];	//0x00
	char nickname[128];	//nickname
	char ssid[32];		//connected to which route
	char ip[16];		//device IP address
	char version1[8];	//0.0.0
	char version2[8];	//2.1.5
	char version3[8];	//e_1.3.00
	char version4[8];	//G_1.0.00
	char key[32];		//UDISCOVERY_UWT
#define HAIER_DISCOVER_RESPONSE_MAGIC	"UDISCOVERY_UWT"
}__attribute__ ((packed))haier_discover_resp_t;

/************************************************************************/
/************************************************************************/
/************************************************************************/
#define HAIER_HEARTBEAT_PERIOD	5	//5 seconds

//0000   00 00 5d f2 00 00 00 00 00 00 00 07 00 00 00 30  ..]............0
//0010   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0020   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0030   30 30 30 37 41 38 xx xx xx xx xx xx 00 00 00 00  0007A8xxxxxx....
typedef struct{			//APP->Device: TCP:56800
	uint32_t type;		//0x5df2
	uint32_t RSV1;
	uint32_t serial;
	uint32_t remain;	//remained data length
	char RSV2[32];
	char MAC[16];
}__attribute__ ((packed))Haier_heartbeat_t;
#define HAIER_HEARTBEAT_INITIAL \
{				\
	.type = 0x5df2,		\
	.RSV1 = 0,		\
	.serial = 0,		\
	.remain = 0x30,		\
}

#define HAIER_WATERHEAT_CMD_HEARTBEAT		0x5df2
#define HAIER_WATERHEAT_CMD_HEARTBEAT_RESPONSE	0x5df3
#define HAIER_WATERHEAT_CMD_CONTROL		0x2714
#define HAIER_WATERHEAT_CMD_CONTROL_RESPONSE	0x2715


//0000   00 00 5d f3 00 00 00 00 00 00 00 07 00 00 00 34  ..]............4
//0010   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0020   00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
//0030   30 30 30 37 41 38 xx xx xx xx xx xx 00 00 00 00  0007A8xxxxxx....
//0040   00 00 00 01                                      ....
typedef struct{			//Device->APP: TCP:56800
	uint32_t type;		//0x5df3
	uint32_t RSV1;		//
	uint32_t serial;	//=heartbrat::serial
	uint32_t remain;	//remained data length
	char RSV2[32];
	char MAC[16];
	uint32_t val;		//0x00000001
}__attribute__ ((packed))Haier_heartbeat_ack_t;

typedef struct{			//Device<->APP: TCP:56800
	uint32_t type;		//0x2714: Control; 0x2715: Response or status
	char RSV2[36];
	char MAC[16];
	char RSV3[16];
	uint32_t serial;	//response to the cmd with the serial number, 0 means not a response
	uint32_t remain;	//remain length
	uint8_t data[];		
}__attribute__ ((packed))Haier_header_t;

typedef struct{
	uint8_t length;		//command length
	char RSV1[6];		//reserved
	uint8_t type;		//command type
#define HAIER_CMD_TYPE_SET_CMD		0x01
#define HAIER_CMD_TYPE_GET_STAT		0x09	//get stat
#define HAIER_CMD_TYPE_SET_RESERVE	0x60

#define HAIER_CMD_TYPE_STAT		0x06
#define HAIER_CMD_TYPE_RESP_STAT	0x02	//new stat of command execute result
	char data[];
}__attribute__ ((packed))Haier_cmd_t;


typedef struct{
	uint8_t cmd2;		//0x5d
	uint8_t cmd;		//cmd type
#define HAIER_SWITCH		0x01		//data: 00: off; 01: on
#define HAIER_TEMPERATURE	0x02		//data: dist temperature
	#define HAIER_TEMPERATURE_MIN	35
	#define HAIER_TEMPERATURE_MAX	75
#define HAIER_POWER		0x08		//data: 1: 800w; 2:1200w; 3:800w+1200w
#define HAIER_RESERVE		0x0f		//data: 1: disable; 0: enable
	uint8_t zero;		//0x00
	uint8_t data;		//data
}__attribute__ ((packed))Haier_control_t;

typedef struct{
	uint8_t length;		//command length
	char RSV1[6];
	uint8_t cmd1;		//0x60
	
	uint8_t cmd2;		//0x00
	uint8_t index;		//reserve index: 1: reserve1; 2: reservel 2
	uint16_t hour;		//hour
	uint16_t minute;	//minute
	uint16_t temperature;	//temperature
	
	uint16_t status;	//02: disable; 01: enable
	uint8_t checksum;	//checksum
}__attribute__ ((packed))Haier_reserve_t;

#define HAIER_STATREPORT_PERIOD	60
typedef struct{
	uint8_t cmd2;		//0x6d				;0x90
	uint8_t cmd;		//0x01
	uint16_t current_temp;	//current temerature
	uint16_t dest_temp;	//dest temperature
	uint16_t hour;		//device time: hour
	
	uint16_t minute;	//device time: minute		;0x98
	uint16_t RSV[2];	//0x00
	uint16_t RSV2;		//0x003c
	
	uint16_t RSV3;		//0x5000			;0xA0
	uint16_t RSV4;		//0x0000
	uint8_t current_temp2;	
	uint8_t RSV5[3];
	
#define HAIER_STAT_HOUR_MASK	0x3F
#define HAIER_STAT_HOUR_ENABLE	(1<<6)
	uint8_t hour_1;		//hour 1	;b6: 1:disable; 0:enable	;0xA8
	uint8_t minute_1;	//minute 1
	uint8_t temp_1;		//temperature of reserve 1
	uint8_t hour_2;		//hour 2	;b6: 1:disable; 0:enable
	uint8_t minute_2;	//minute 2
	uint8_t temp_2;		//temperature of reserve 2
	uint16_t RSV6;		//0x4000
	
	uint32_t RSV7;		//0x00400000			;0xB0
	uint32_t RSV8;		//0x40000040
	
	uint8_t RSV9[3];	//				;0xB8
	uint8_t status;		//d1: 高速  91：中速  51：低速（b7：1200w，b6：800w，）b2: 预约？b0(0:关机, 1开机) c5:timer1+timer2  C3:timer2 disable
#define HAIER_STAT_STATUS_1200W		(1<<7)
#define HAIER_STAT_STATUS_800W		(1<<6)
#define HAIER_STAT_STATUS_RESERVE	(3<<1)
#define HAIER_STAT_STATUS_ON		(1<<0)
	uint8_t RSV10;		//0x80
	uint8_t status2;	//01: 加热中；00：保温  21:动态增容
#define HAIER_STAT_STATUS_HEATING	(1<<0)
#define HAIER_STAT_STATUS_DYNAMIC	(1<<5)
	uint8_t RSV11[2];	//0x00
	
	uint8_t RSV12[8];	//				;0xC0
	
	uint32_t RSV13;		//0x80000600			;0xC8
	uint16_t RSV14;		//00
	uint8_t checksum;
}__attribute__ ((packed))Haier_stat_t;
#define HAIER_STAT_INIT(stat)		(*(volatile long*)(stat) = 0)
#define HAIER_STAT_INVALID(stat)	(0==(*(volatile long*)(stat)))


#endif //__HAIER_WATERHEATER_H__
