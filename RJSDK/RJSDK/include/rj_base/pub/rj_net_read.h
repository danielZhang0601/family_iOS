///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_net_read.h
/// @brief	 管理网络包的内存块（读）
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_READ_H__
#define __RJ_NET_READ_H__

#include "util/rj_type.h"
#include "pub/pub_type.h"

typedef struct rj_net_read_t rj_net_read_t;
typedef rj_net_read_t* rj_net_r_h;

/// @brief 初始化内存池
/// @param [in] p_block  内存块信息
/// @return rj_net_r_h      返回句柄，若创建失败则返回NULL。
RJ_API rj_net_r_h rj_net_read_create(_RJ_MEM_BLOCK_ *p_block, int ws);

/// @brief 销毁句柄
/// @param [in] handle      句柄
/// @return _RJ_MEM_BLOCK_ *      内存块信息
RJ_API _RJ_MEM_BLOCK_ * rj_net_read_destroy(rj_net_r_h handle);

/// @brief 提取一块数据
/// @param [in] handle      句柄
/// @param [in] *p_data      数据
/// @return uint32     数据长度
RJ_API uint32 rj_net_read_pop(rj_net_r_h handle, char **pp_data);
#endif
//__RJ_NET_READ_H__