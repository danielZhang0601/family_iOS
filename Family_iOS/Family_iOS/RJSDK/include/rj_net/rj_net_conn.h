///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/17
//
/// @file	 rj_net_conn.h
/// @brief	 定义网络组件的相关定义
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_CONN_H__
#define __RJ_NET_CONN_H__

#include "util/rj_type.h"
#include "pub/rj_ndp.h"
#include "pub/rj_net_read.h"
#include "pub/rj_net_write.h"
#include "uv.h"
#include "rn_tcp.h"


typedef struct rj_net_conn_t rj_net_conn_t;
typedef rj_net_conn_t* rj_net_conn_h;

typedef struct rj_net_multi_conn_t rj_net_multi_conn_t;
typedef rj_net_multi_conn_t* rj_net_m_conn_h;

/// @brief 创建一个连接
/// @return rj_net_conn_h 句柄
RJ_API rj_net_conn_h rj_conn_create(rj_net_m_conn_h m_conn, uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);

/// @brief 销毁一个连接
/// @param [in] handle 句柄
RJ_API void rj_conn_destroy(rj_net_conn_h handle);

/// @brief 获取连接ID
/// @param [in] handle 句柄
/// @return uint32 连接ID
RJ_API int rj_conn_id(rj_net_conn_h handle);


/// @brief 设置连接ID
/// @param [in] handle  句柄
/// @param [in] id      id值
RJ_API void rj_conn_set_id(rj_net_conn_h handle, int id);


/// @brief 停止一个连接
/// @param [in] handle 句柄
RJ_API void rj_conn_stop(rj_net_conn_h handle);

/// @brief 插入一个连接通道
/// @param [in] handle 句柄
/// @param [in] tag 通道ID
/// @param [in] p_w_conn websocket的句柄
/// @return 0：成功，1：失败
RJ_API int rj_conn_push_ch(rj_net_conn_h handle, rn_tcp_h ws_tcp);

/// @brief 弹出一个连接通道
/// @param [in] handle 句柄
/// @param [in] ch_id 通道ID
RJ_API void rj_conn_pop_ch(rj_net_conn_h handle, int ch_id);

/// @brief 发送一块数据（有多个RJ包）
/// @param [in] handle 句柄
/// @param [in] ch_id      指定的网络通道
/// @param [in] *p_ndp      NDP网络包头
/// @param [in] *p_data      数据指针（长度见p_ndp）
/// @return int 0:成功；非0,其他错误
RJ_API int rj_conn_send(rj_net_conn_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief 接收一块数据
/// @param [in] handle 句柄
/// @param [out] *p_recv_data 数据缓存句柄的指针
/// @return int 0:成功；1：失败
RJ_API int rj_conn_recv(rj_net_conn_h handle, rj_net_r_h *p_recv_data);

/// @brief 释放一块内存
/// @param [in] handle 句柄
/// @param [out] recv_data 数据缓存句柄
/// @return int 0:成功；1：失败
RJ_API void rj_conn_free_mem(rj_net_conn_h handle, rj_net_r_h recv_data);



#endif//__RJ_NET_CONN_H__
