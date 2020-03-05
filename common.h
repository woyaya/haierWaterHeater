#ifndef __COMMON_H__
#define __COMMON_H__
#define _GNU_SOURCE

#ifdef OS_LINUX
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <stdio.h>

#ifndef U32
  typedef unsigned int U32;
#endif
#ifndef U16
  typedef unsigned short U16;
#endif
#ifndef U8
  typedef unsigned char U8;
#endif
#ifndef S32
  typedef signed int S32;
#endif
#ifndef S16
  typedef signed short S16;
#endif
#ifndef S8
  typedef signed char S8;
#endif

#define LOG_INIT(...) do{}while(0)
 
#define LOG_LEVEL_NOISY		0
#define LOG_LEVEL_DEBUG		1
#define LOG_LEVEL_INFO		2
#define LOG_LEVEL_NOTICE	3
#define LOG_LEVEL_WARNING	4
#define LOG_LEVEL_ERROR		5
#define LOG_LEVEL_BUG		6

#ifndef LOG_DEFAULT_LEVEL
  #define LOG_DEFAULT_LEVEL	LOG_LEVEL_INFO
#endif
/*********************************************/
#if defined(DEBUG) || defined(_DEBUG)
  #ifndef HX_DEBUG
    #define HX_DEBUG 1
  #endif
#endif	
/*********************************************/
#if defined(__KERNEL__) || defined(MODULE)
  #include <linux/kernel.h>
  #define HX_PRINT_D(x...)	printk(x)
  #define HX_PRINT_I(x...)	printk(x)
  #define HX_PRINT_N(x...)	printk(x)
  #define HX_PRINT_W(x...)	printk(x)
  #define HX_PRINT_E(x...)	printk(x)
  #define HX_STD_ERR		KERN_INFO
#elif defined(HAVE_SYSLOG)
  #include <syslog.h>
  #define HX_PRINT_D(x...)	syslog(LOG_DEBUG,x)
  #define HX_PRINT_I(x...)	syslog(LOG_INFO,x)
  #define HX_PRINT_N(x...)	syslog(LOG_NOTICE,x)
  #define HX_PRINT_W(x...)	syslog(LOG_WARNING,x)
  #define HX_PRINT_E(x...)	syslog(LOG_ERR,x)
  #define HX_STD_ERR
#elif defined(X86_SIMULATION_LOG)
  #include "sim_driver.h"
  #define HX_PRINT_D(x...)	app_printf(x)
  #define HX_PRINT_I(x...)	app_printf(x)
  #define HX_PRINT_N(x...)	app_printf(x)
  #define HX_PRINT_W(x...)	app_printf(x)
  #define HX_PRINT_E(x...)	fprintf(stderr,x)
#else
  #include <stdio.h>
  #define HX_PRINT_D(x...)	printf(x)
  #define HX_PRINT_I(x...)	printf(x)
  #define HX_PRINT_N(x...)	printf(x)
  #define HX_PRINT_W(x...)	printf(x)
  #define HX_PRINT_E(x...)	fprintf(stderr,x)
#endif
extern unsigned short log_level;
/*********************************************/
#if defined(HX_NOISY)
  #define NOISY(fmt, args...)	\
	do{			\
		if (log_level <=  LOG_LEVEL_NOISY)\
			HX_PRINT_D("[NOISY@%s:%d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
#else
  #define NOISY(fmt, args...)
#endif
#if defined(HX_DEBUG)
  #define DBG(fmt, args...)	\
	do{			\
		if (log_level <= LOG_LEVEL_DEBUG)\
			HX_PRINT_D("[DBG@%s:%d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
#else
  #define DBG(fmt, args...)
#endif

#ifndef INFO
  #define INFO(fmt, args...)	\
	do{			\
		if (log_level <= LOG_LEVEL_INFO)\
			HX_PRINT_I("[MSG@%s:%d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
  #define MSG	INFO
#endif

#ifndef NOTICE
  #define NOTICE(fmt, args...)	\
	do{			\
		if (log_level <= LOG_LEVEL_NOTICE)\
			HX_PRINT_N("[MSG@%s:%d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
#endif

#ifndef WRN
  #define WRN(fmt, args...)	\
	do{			\
		if (log_level <= LOG_LEVEL_WARNING)\
			HX_PRINT_W("[WRN@%s:%d]: "fmt"\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
#endif

#ifndef ERR
  #define ERR(fmt, args...)	\
	do{			\
		if (log_level <= LOG_LEVEL_ERROR)\
			HX_PRINT_E("[ERR@%s:%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args);\
	}while(0)
#endif

#ifndef BUG
  #define BUG(fmt, args...)		\
	do{				\
		HX_PRINT_E("[BUGR@%s:%d]: " fmt "\n", __FUNCTION__, __LINE__, ##args);\
		while(1)		\
			sleep(1);	\
	}while(0)
#endif
/*********************************************/
#ifndef likely
  #define likely(x)	__builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
  #define unlikely(x)	__builtin_expect(!!(x), 0)
#endif
/*********************************************/
#if !defined NO_PRAGMA && defined __GNUC__ && (__GNUC__ > 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ > 4)))
#ifndef GCC_PUSH_OPTION
  #define GCC_PUSH_OPTION		_Pragma("GCC push_options")
#endif
#ifndef GCC_POP_OPTION
  #define GCC_POP_OPTION		_Pragma("GCC pop_options")
#endif
#ifndef GCC_OPTIMIZE_O0
  #define GCC_OPTIMIZE_O0		_Pragma("GCC optimize (\"O0\")")
#endif
#else   //__GNUC__
#ifndef GCC_PUSH_OPTION
  #define GCC_PUSH_OPTION
#endif
#ifndef GCC_POP_OPTION
  #define GCC_POP_OPTION
#endif
#ifndef GCC_OPTIMIZE_O0
  #define GCC_OPTIMIZE_O0
#endif
#endif  //__GNUC__

#if defined HAVE_ATTRIBUTE_OPTIMIZE
  #define ATTRIBUTE_OPTIMIZE_00		__attribute__ ((__used__))
//#define ATTRIBUTE_OPTIMIZE_00		__attribute__((optimize("-O0")))
  #define ATTRIBUTE_OPTIMIZE_01		__attribute__((optimize("-O1")))
  #define ATTRIBUTE_OPTIMIZE_02		__attribute__((optimize("-O2")))
  #define ATTRIBUTE_OPTIMIZE_0s		__attribute__((optimize("-Os")))
#else	//HAVE_ATTRIBUTE_OPTIMIZE
  #define ATTRIBUTE_OPTIMIZE_00
  #define ATTRIBUTE_OPTIMIZE_01
  #define ATTRIBUTE_OPTIMIZE_02
  #define ATTRIBUTE_OPTIMIZE_0s
#endif	//HAVE_ATTRIBUTE_OPTIMIZE
/*********************************************/
#ifndef __STRING
  #define __STRING(x)	#x
#endif
#ifndef STRING
  #define STRING(x)	__STRING(x)
#endif
#ifndef __PASTE
  #define __PASTE(x,y)	x ## y
#endif
#ifndef PASTE
  #define PASTE(x,y)	__PASTE(x,y)
#endif
#ifndef UNIQUE
  #ifdef __GNUC__
     #define UNIQUE(x)	PASTE(x, PASTE(__COUNTER__,__LINE__))
  #else
     #define UNIQUE(x)	PASTE(x, __LINE__)
  #endif
#endif
#ifndef BUILD_TIME_CONDITION
  #define BUILD_TIME_CONDITION(ARRAY, VALUE1, VALUE2)	\
	((NULL==(ARRAY))?(VALUE1):(VALUE2))
#endif
/*********************************************/
#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(x)	(sizeof(x)/sizeof(x[0]))
#endif
/*********************************************/
#ifndef GETTID
	#define GETTID()	syscall(SYS_gettid)
#endif
/*********************************************/
#ifndef PRINTABLE
	#define PRINTABLE(S)	((NULL==(S)) ? "" : (S))
#endif
/*********************************************/
#ifndef SECTION_ALIGN_SIZE
  #if defined(CPU_X86_64) || defined(CPU_X64)
    #define SECTION_ALIGN_SIZE		32
  #else
    #define SECTION_ALIGN_SIZE		4
  #endif
#endif

#ifndef SECTION_ALIGNED
  #define SECTION_ALIGNED	__attribute__((aligned(SECTION_ALIGN_SIZE)))
#endif
#ifndef STRUCT_PACKED
  #define STRUCT_PACKED	__attribute__((packed))
#endif
/*********************************************/
#ifndef SECTION
  #define SECTION(NAME) __attribute__((used,section(STRING(NAME))))
#endif
#ifndef WEAK_SYMBOL
  #define WEAK_SYMBOL	__attribute__((weak))
#endif
#ifndef WEAK_REF
  #define WEAK_REF(NAME)	__attribute__((weakref(STRING(NAME))))
#endif
#ifndef WEAK_ALIAS
  #define WEAK_ALIAS(NAME)	__attribute__((weak,alias(STRING(NAME))))
#endif
#ifndef STRONG_ALIAS
  #define STRONG_ALIAS(NAME)	__attribute__((alias(STRING(NAME))))
#endif
#ifndef MAY_UNUSED
  #define MAY_UNUSED		__attribute__((unused))
#endif
/*********************************************/
#endif
