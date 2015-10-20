////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/05
//
/// @file	 logger.h
/// @brief	 使用说明：
///   1、 打印分为5个级别，5：调试  4：正常  3：警告  2：错误  1：严重错误 
///       调试打印 使用LOG_DEBUG();
///       正常打印 使用LOG_INFO();
///       警告打印 使用LOG_WARN();
///       错误打印 使用LOG_ERROR();
///       严重错误打印 使用LOG_FATAL();  系统出现非常严重的问题使用该打印，一般的错误采用LOG_ERROR
///   2、如果单个模块需要提高打印级别。
///   假如要将某个模块的打印由调试级别按正常级别打印，可以在该模块中的所有cpp文件头加如下内容
/// #undef   LOG_DEBUG
/// #define  LOG_DEBUG LOG_INFO

/// @author  jmb
/// @version 0.1
//////////////////////////////////////////////////////////////////////////////////////////////////



#ifndef _RJ_LOGGER_H_
#define _RJ_LOGGER_H_
#include <stdio.h>
#include <string.h>
#include "util/rj_type.h"


#define LOG_LEVEL_FATAL  1
#define LOG_LEVEL_ERROR  2
#define LOG_LEVEL_WARN   3
#define LOG_LEVEL_INFO   4
#define LOG_LEVEL_DEBUG  5

#define LOG_LEVEL_ALL    LOG_LEVEL_DEBUG

#ifndef LOG_LEVEL
	#ifndef NDEBUG
	#define LOG_LEVEL LOG_LEVEL_DEBUG
	#else
	#define LOG_LEVEL LOG_LEVEL_INFO
	#endif
#endif

#define LOG_TITLE_FATAL  "FATAL"
#define LOG_TITLE_ERROR  "ERROR"
#define LOG_TITLE_WARN   "WARN"
#define LOG_TITLE_INFO   "INFO"
#define LOG_TITLE_DEBUG  "DEBUG"


#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)


#ifdef   __RJ_WIN__

#ifndef TRACE
#define TRACE printf
#endif

#define __LOG_PRINTF__(format, ...) printf(format, ## __VA_ARGS__)

#elif defined __RJ_LINUX__

#define  MAX_LOG_LEN   1024

#define __LOG_PRINTF__(format, ...) printf(format, ## __VA_ARGS__)

#elif defined __RJ_IOS__

#define  MAX_LOG_LEN   1024

#define __LOG_PRINTF__(format, ...) printf(format, ## __VA_ARGS__)

#elif  defined __RJ_ANDROID__
#include <android/log.h>
#define __LOG_PRINTF__(format, ...) __android_log_print(format,## __VA_ARGS__) 

#else 
#define __LOG_PRINTF__(format, ...) printf(format, ## __VA_ARGS__)
#endif


#define __LOG__(title, format, ...) __LOG_PRINTF__(__FILE__ "(" __STR1__(__LINE__) "): " title ": " format, ## __VA_ARGS__)


// LOG FATAL
#if LOG_LEVEL >= LOG_LEVEL_FATAL
#define LOG_FATAL(format, ...) __LOG__(LOG_TITLE_FATAL, format, ## __VA_ARGS__)
#else
#define LOG_FATAL(format, ...)
#endif


// LOG ERROR
#if LOG_LEVEL >= LOG_LEVEL_ERROR

#define LOG_ERROR(format, ...) __LOG__(LOG_TITLE_ERROR, format, ## __VA_ARGS__)

#else
#define LOG_ERROR(format, ...)
#endif

// LOG WARN
#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(format, ...) __LOG__(LOG_TITLE_WARN, format, ## __VA_ARGS__)
#else
#define LOG_WARN(format, ...)
#endif


// LOG INFO
#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(format, ...) __LOG__(LOG_TITLE_INFO, format, ## __VA_ARGS__)
#else
#define LOG_INFO(format, ...)
#endif

// LOG DEBUG
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(format, ...) __LOG__(LOG_TITLE_DEBUG, format, ## __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif


void LOG_TIME_PRINTF(const char *pszFmt, ...);

//void LOG_WIN_PRINTF(const char *pszFmt, ...);

#endif//_LOGGER_H_



