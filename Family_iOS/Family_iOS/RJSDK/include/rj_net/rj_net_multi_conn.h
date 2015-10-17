///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/17
//
/// @file	 rj_net_multi_conn.h
/// @brief	 定义多个网络连接
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_MULTI_CONN_H__
#define __RJ_NET_MULTI_CONN_H__

#include "util/rj_type.h"
#include "pub/rj_ndp.h"
#include "pub/rj_net_read.h"
#include "uv.h"
#include "rn_tcp.h"


typedef struct rj_net_multi_conn_t rj_net_multi_conn_t;
typedef rj_net_multi_conn_t* rj_net_m_conn_h;


/// @brief 创建一个多连接管理器
/// @param [in] s_sbl       发送缓存内存块的大小
/// @param [in] sbl         发送缓存总的大小
/// @param [in] s_rbl       接收缓存内存块的大小
/// @param [in] s_sbl       发送缓存总的大小
/// @return rj_net_m_conn_h 句柄
RJ_API rj_net_m_conn_h rj_m_conn_create(uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);


/// @brief 销毁一个多连接管理器
/// @param [in] handle 句柄
RJ_API void rj_m_conn_destroy(rj_net_m_conn_h handle);


/// @brief 停止多连接管理
/// @param [in] handle 句柄
RJ_API void rj_m_conn_stop(rj_net_m_conn_h handle);


/// @brief 停止连接
/// @param [in] handle 句柄
/// @param [in] conn_id 连接ID
RJ_API void rj_m_conn_stop_conn(rj_net_m_conn_h handle, int conn_id);


/// @brief 插入一个连接通道
/// @param [in] handle  句柄
/// @param [in] conn_id 连接ID
/// @param [in] p_w_conn websocket的句柄
/// @return 0:成功; 1:失败, 调用者另自行处理ws_tcp
RJ_API int rj_m_conn_push_ch(rj_net_m_conn_h handle, int conn_id, rn_tcp_h ws_tcp);


/// @brief 弹出一个连接通道
/// @param [in] handle  句柄
/// @param [in] conn_id 连接ID
/// @param [in] ch_id   通道ID
RJ_API void rj_m_conn_pop_ch(rj_net_m_conn_h handle, int conn_id, int ch_id);


/// @brief 发送一块数据(有多个RJ包)
/// @param [in] handle      句柄
/// @param [in] conn_id     连接ID
/// @param [in] net_ch_id   通道ID
/// @param [in] *p_ndp      NDP网络包头
/// @param [in] *p_data     数据指针(长度见p_ndp)
/// @return int 0:成功; 非0:错误
RJ_API int rj_m_conn_send(rj_net_m_conn_h handle, int conn_id, int net_ch_id, rj_ndp_pk_t *p_ndp, char *p_data);


/// @brief 接收一块数据
/// @param [in] handle          句柄
/// @param [out] *p_conn_id     连接ID的指针
/// @param [out] *p_recv_data   数据缓存句柄的指针
/// @return int 0:成功; 非0,错误
/// @note 如果获取数据不及时, 会导致接收缓存被占满, 时间过长会出现TCP断开
RJ_API int rj_m_conn_recv(rj_net_m_conn_h handle, int *p_conn_id, rj_net_r_h *p_recv_data);


/// @brief 释放一块内存
/// @param [in] handle      句柄
/// @param [in] conn_id     连接ID
/// @param [out] recv_data  数据缓存句柄
/// @return int 0:成功; 1:失败
RJ_API void rj_m_conn_free_mem(rj_net_m_conn_h handle, int conn_id, rj_net_r_h recv_data);


#endif//__RJ_NET_MULTI_CONN_H__
