#!/bin/sh
NAME=haier
IFNAME=`ls /sys/class/net | head -n 1`
MAIL_ADDRESS="4523468@qq.com"
MAIL_TITLE="Haier waterheater setting"

#log=/tmp/`basename $0`.$$
log=/tmp/`basename $0`.log
DIR=`which $NAME`
[ $? != 0 ] && EXEC=`dirname $0`/$NAME || EXEC=$NAME

USAGE(){
	echo "Usage:"
	echo "$1 -T temperature -l -m device_MAC"
	echo "	-s on/off: on: turn on; off: turn off"
	echo "	-T temperature: 0: power off; [35,75]: temperature"
	echo "	-l count: loop for 'count' times if fail, '0': loop forever. defalut: 0."
	echo "	-m device_MAC: water heater mac address"
}
LOG_RESIZE(){
	tail -n 100 $log >$log.tmp
	mv $log.tmp $log
}
LOG(){
	logger -s "$$: $@"
	echo "$$: $@" >>$log
}
ERR(){
	LOG "$@"
	LOG_RESIZE
#	cat $log | mail -s "$MAIL_TITLE FAIL" $MAIL_ADDRESS
#	send mail

	exit 1
}

[ $# = 0 ] && {
	USAGE $0
	exit 1
}
LOG "`date`"

temperature=""
switch=""

loop=10
while getopts ":T:l:m:s:" opt; do
	case $opt in
		s)
			switch=$OPTARG
		;;
		T)
			temperature=$OPTARG
		;;
		l)
			loop=$OPTARG
		;;
		m)
			mac=$OPTARG
		;;
		\?)
			USAGE $0
			exit 0
		;;
		*)
			USAGE $0
			exit 1
		;;
	esac
done

if [ -z "$temperature" -a -z "$switch" ] || [ -z "$mac" ]
then
	USAGE $0
	exit 1
fi
[ "$temperature" = "0" ] && switch="off"
[ "$switch" = "off" ] && temperature=""
[ -n "$temperature" ] && switch="on"
LOG "Local ifname:$IFNAME, device mac:$mac, dist switch: $switch, dist temperature:$temperature, loop:$loop"

GET_RESULT(){
	echo "$STAT" | sed "/$1:/!d;s/.*://"
}
CHECK_RESULT(){
	local VALUE
	VALUE=`GET_RESULT $1`
	[ "$VALUE" = "$2" ] && return 0
	return 1
}
RETRY(){
	local retry=$1;shift
	local cmd="$@"
	[ $retry = 1 ] && ERR "$mac: Execute command \"$@\" fail"
	LOG "$retry: Command \"$@\" fail, retry "
	sleep 5	#heartbeat period
}
UPDATE_STAT(){
	local retry=$loop
	while true;do
		STAT=`$EXEC -j -I $IFNAME -m $mac -g 1 -s 2 2>>/dev/null | sed '/^\t"/!d;s/[\t",]//g'`
		[ -n "$STAT" ] && break;
		RETRY $retry "Get stat"
		[ $retry != 0 ] && retry=`expr $retry "-" 1`
	done
}
EXECUTE_CMD(){
	local result
	local retry=$loop
	LOG "$retry: $EXEC -j -I $IFNAME -m $mac $@"
	while true;do
		result=`$EXEC -j -I $IFNAME -m $mac $@ | sed '/^\t"/!d;s/[\t",]//g'`
		[ $? = 0 ] && {
			[ -n "$result" ] && STAT="$result"
			break;
		}
		RETRY $retry "Exec $@"
		[ $retry != 0 ] && retry=`expr $retry "-" 1`
	done
	sleep 1
}

retry=$loop
UPDATE_STAT
while true;do
	SWITCH=`GET_RESULT Switch`
	[ -n "$switch" ] && {
		[ "$SWITCH" != "$switch" ] && {
			case $switch in
				off)
					EXECUTE_CMD -o 0
				;;
				on)
					EXECUTE_CMD -o 1
				;;
				*)
					ERR "Unsupported param: \"$switch\""
				;;
			esac
		}
		PARAM="Switch $switch"
	}
	[ -n "$temperature" ] && {
		RESERVE=`GET_RESULT Reserve[12]`
		[ -n "$RESERVE" ] && EXECUTE_CMD -r 0
		TEMPERATURE=`GET_RESULT DistTemperature`
		LOG "Change temperature from '$TEMPERATURE' to '$temperature'"
		[ $TEMPERATURE = $temperature ] && break
		EXECUTE_CMD -T $temperature
		PARAM="DistTemperature $temperature"
		sleep 2
	}
	CHECK_RESULT $PARAM
	[ $? = 0 ] && break
	RETRY $retry "Check result with $PARAM"
	[ $retry != 0 ] && retry=`expr $retry "-" 1`
done

LOG "Success: $@"
LOG_RESIZE
#rm -rf $log
