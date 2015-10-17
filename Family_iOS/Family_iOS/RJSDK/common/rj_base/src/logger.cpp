///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/01
//
/// @file	 rj_logger.cpp
/// @brief	 文件简要描述
/// @author  jmb
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#include "util/logger.h"
#include <stdarg.h>
#include <time.h>
#include "sys/sys_missing.h"
//#include   <windows.h>

const int RJ_MAX_LOG_LEN = 1024;

#ifdef __RJ_WIN__
//void LOG_WIN_PRINTF(const char *pszFmt, ...)
//{
//    va_list vaList;
//    int lBufLen= RJ_MAX_LOG_LEN;
//    char szBuf[RJ_MAX_LOG_LEN]={0};
//    va_start(vaList, pszFmt);
//    vsnprintf(szBuf, lBufLen-1, pszFmt, vaList);
//    va_end(vaList);
//    OutputDebugString(szBuf);
//}
#endif

#ifdef __RJ_LINUX__
void LOG_TIME_PRINTF(const char *pszFmt, ...)
{
    va_list vaList;
    int   iLen =0;
    int lBufLen= RJ_MAX_LOG_LEN;
    char buf[64]={0};
    char szBuf[RJ_MAX_LOG_LEN]={0};
    struct tm *t;
    time_t tt;
    time(&tt);
    t=localtime(&tt);
    sprintf(buf,"%4d-%02d-%02d %02d:%02d:%02d ",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);

    iLen = snprintf(szBuf, lBufLen, "%s ",buf);
    va_start(vaList, pszFmt);
    iLen += vsnprintf(szBuf+iLen, lBufLen-iLen, pszFmt, vaList);
    va_end(vaList);
    printf("%s",szBuf);
}
#endif