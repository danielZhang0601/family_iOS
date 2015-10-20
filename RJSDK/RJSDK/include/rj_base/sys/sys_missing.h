///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/08/31
//
/// @file	 sys_missing.h
/// @brief	 屏蔽平台差异定义, 勿定义其他信息
/// @author  lsl
/// @version 0.1
/// @history 修改历史
///  \n lsl	2015/08/31	0.1	创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __SYS_MISSING_H__
#define __SYS_MISSING_H__

#include <stdio.h>
#include "util/rj_type.h"


#ifdef __RJ_WIN__
	#ifndef snprintf
		#define snprintf _snprintf
	#endif
#elif defined(__RJ_LINUX__)

	#ifndef max
		#define max(a,b)    (((a) > (b)) ? (a) : (b))
	#endif

	#ifndef min
		#define min(a,b)            (((a) < (b)) ? (a) : (b))
	#endif
#else
	#error Not Suport Platform
#endif


#endif // __SYS_MISSING_H__
//end
