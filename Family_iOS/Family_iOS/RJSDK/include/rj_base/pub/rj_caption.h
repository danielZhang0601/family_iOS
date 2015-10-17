///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/10
//
/// @file	 rj_caption.h
/// @brief	 管理一种类型的字符串
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_CAPTION_H__
#define __RJ_CAPTION_H__

typedef struct rj_cap_t rj_cap_t;
typedef rj_cap_t * rj_cap_h;

rj_cap_h rj_cap_create();

void rj_cap_destroy(rj_cap_h cap_h);

void rj_cap_push(rj_cap_h cap_h, int index, const char *p_caption);

const char * rj_caption(rj_cap_h cap_h, int index);

int rj_cap_index(rj_cap_h cap_h, const char *p_caption);
#endif
//__RJ_CAPTION_H__
