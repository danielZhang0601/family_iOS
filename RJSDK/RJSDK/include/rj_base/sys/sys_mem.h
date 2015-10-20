///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/08/20
//
/// @file	 sys_mem.h
/// @brief	 ���� malloc/free, ��Ҫ��Ϊ�����ڴ�й¶����
/// @author  lsl
/// @version 0.1
/// @history �޸���ʷ
///  \n lsl	2015/08/20	0.1	�����ļ�
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_MEM_H__
#define __RJ_MEM_H__

#include "util/rj_type.h"



RJ_API void* sys_my_malloc(int size, const char *file, const char *func, int line);
RJ_API void  sys_my_free(void *ptr);



#ifdef NDEBUG

RJ_API void* sys_malloc(int size);
RJ_API void  sys_free(void *ptr);

#else

#undef sys_malloc
#undef sys_free

#define sys_malloc(SYS_MEM_SIZE)	sys_my_malloc((SYS_MEM_SIZE), __FILE__, __FUNCTION__, __LINE__)
#define sys_free(SYS_MEM_PTR)		sys_my_free(SYS_MEM_PTR)

#endif


#endif // __RJ_MEM_H__
//end
