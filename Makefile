#########################
#Change following setting if needed
PLATFORM := UBUNTU
#PLATFORM := OPENWRT
####
EXEC := haier				#Final executable name
#Compiler with debug info or not
#DEBUG := 1
#Source code of openwrt firmware
OPENWRT_SRC_DIR := ./WRT1200
ifeq ($(PLATFORM),UBUNTU)
  DEFAULT_IFNAME := enp2s1
endif
ifeq ($(PLATFORM),OPENWRT)
  DEFAULT_IFNAME := br-lan
endif
#########################
ifeq ($(PLATFORM),OPENWRT)
#Compiler can be download from 
#	http://downloads.openwrt.org/releases/19.07.1/targets/mvebu/cortexa9/openwrt-sdk-19.07.1-mvebu-cortexa9_gcc-7.5.0_musl_eabi.Linux-x86_64.tar.xz
CROSS_COMPILER := $(OPENWRT_SRC_DIR)/staging_dir/toolchain-*/bin/arm-openwrt-linux-
endif
CC := $(CROSS_COMPILER)gcc
STRIP := $(CROSS_COMPILER)strip

SRCS := $(filter-out main.c,$(wildcard *.c))
OBJS := $(patsubst %.c,%.o,$(SRCS))

CFLAGS += -Wall -Werror
CFLAGS += -I.
CFLAGS += -DDEFAULT_IFNAME=\"$(DEFAULT_IFNAME)\"

ifeq ($(DEBUG),1)
  CFLAGS += -g
else
  CFLAGS += -O2
endif

LDFLAGS += -lpthread

all : main.c $(OBJS)
	$(CC) $(CFLAGS) $^ -o $(EXEC) $(LDFLAGS)
ifneq ($(DEBUG),1)
	$(STRIP) $(EXEC)
endif

clean:
	rm -rf $(OBJS) $(EXEC)
