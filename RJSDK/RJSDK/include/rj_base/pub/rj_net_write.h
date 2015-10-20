///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_net_write.h
/// @brief	 管理网络包的内存块（写）
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_WRITE_H__
#define __RJ_NET_WRITE_H__

#include "util/rj_type.h"
#include "pub/pub_type.h"
#include "pub/rj_ndp.h"


typedef _RJ_MEM_BLOCK_* rj_net_w_h;

/// @brief 初始化内存池
/// @param [in] p_block  内存块信息
/// @return rj_net_w_h      返回句柄，若创建失败则返回NULL。
RJ_API rj_net_w_h rj_net_write_create(_RJ_MEM_BLOCK_ *p_block);

/// @brief 销毁句柄
/// @param [in] handle      句柄
/// @return _RJ_MEM_BLOCK_ *      内存块信息
RJ_API _RJ_MEM_BLOCK_ * rj_net_write_destroy(rj_net_w_h handle);

/// @brief 写入一块数据
/// @param [in] handle      句柄
/// @param [in] *p_ndp      NDP网络包头
/// @param [in] *p_data     数据指针（长度见p_ndp）
/// @param [in] ws_type     WS类型（如果不是Websocket，则为-1）
/// @return int     0：写入成功，1：失败。
RJ_API int rj_net_write_push(rj_net_w_h handle, rj_ndp_pk_t *p_ndp, char *p_data, char ws_type);
#endif
//__RJ_NET_WRITE_H__