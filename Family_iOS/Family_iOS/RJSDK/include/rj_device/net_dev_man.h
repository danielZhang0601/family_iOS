///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/21
//
/// @file	 net_dev_man.h
/// @brief	 网络管理服务端
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __NET_DEV_MAN_H__
#define __NET_DEV_MAN_H__

#include "util/rj_type.h"
#include "pub/rj_net_read.h"
#include "rj_net_pre_conn.h"

typedef struct uv_loop_s uv_loop_t;
typedef struct net_dev_man_t net_dev_man_t;
typedef net_dev_man_t* net_dev_man_h;

/// @brief 创建一个网络服务器
/// @param [in] s_sbl   发送缓存内存块的大小
/// @param [in] sbl     发送缓存总的大小
/// @param [in] s_rbl   接收缓存内存块的大小
/// @param [in] s_sbl   发送缓存总的大小
/// @return net_dev_man_h 句柄
RJ_API net_dev_man_h ndm_create(rn_server_h ser_handle,rn_client_h cli_handle,uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);

/// @brief 销毁一个网络服务器
/// @param [in] handle 句柄
RJ_API void ndm_destroy(net_dev_man_h handle);

/// @brief 停止服务
/// @param [in] handle 句柄
RJ_API void ndm_stop(net_dev_man_h handle);

/// @brief 创建一个新的连接通道（在预连接模块中）
/// @param [in] handle 句柄
/// @param [in] p_addr 设备的IP地址
/// @param [in] port 设备的端口号
/// @param [out] p_ch_id 连接ID
/// @return int RN_TCP_OK：成功，其他：失败
RJ_API int ndm_create_ch(net_dev_man_h handle, char *p_addr,unsigned short port,int *p_ch_id);

/// @brief 使能一个新的连接通道，就是把它从预连接通道移到一个连接上去（或者创建一个新的连接）
/// @param [in] handle      句柄
/// @param [in] dev_id      设备ID，由外部统一分配
/// @param [in] ch_id       网络通道ID，由内部产生
/// @return int RN_TCP_OK：成功，其他：失败
RJ_API int ndm_enable_ch(net_dev_man_h handle, int dev_id, int ch_id);

/// @brief 不使能一个连接通道
/// @param [in] handle 句柄
/// @param [in] dev_id      设备ID，由外部统一分配
/// @param [in] ch_id       网络通道ID，由内部产生
RJ_API void ndm_disable_ch(net_dev_man_h handle, int dev_id, int ch_id);

/// @brief 关闭某个设备
/// @param [in] handle 句柄
/// @param [in] dev_id      设备ID，由外部统一分配
RJ_API void ndm_close_device(net_dev_man_h handle, int dev_id);

/// @brief 关闭预连接
/// @param [in] handle      句柄
/// @param [in] ch_id       网络通道ID
RJ_API void ndm_pcon_close_ch(net_dev_man_h handle, int ch_id);

/// @brief 预连接发送数据
/// @param [in] handle      句柄
/// @param [in] ch_id       通道ID
/// @param [in] p_ndp       数据包头
/// @param [in] p_data      数据缓冲区指针
/// @return int RN_TCP_OK：发送成功，RN_TCP_DISCONNECT：连接断开，其他：失败
RJ_API int ndm_pconn_send(net_dev_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief 预连接接收数据
/// @param [in] handle      句柄
/// @param [out] *p_ch_id   通道ID
/// @param [int] *p_buf     缓存指针
/// @param [in] buf_len     缓冲区长度
/// @param [out] p_data_len 接收到的数据长度
/// @return int， RN_TCP_OK：接收数据成功,RN_TCP_DISCONNECT：连接断开，RN_TCP_NEW_CONNECT：有新连接，其他：没数据
RJ_API int ndm_pconn_recv(net_dev_man_h handle, int *p_ch_id, char *p_buf, uint32 buf_len, uint32 *p_data_len);

/// @brief 连接发送其他所有数据
/// @param [in] handle          句柄
/// @param [in] dev_id          返回设备ID
/// @param [in] p_ndp           数据包头
/// @param [in] p_data          数据缓冲区指针
/// @return int， RN_TCP_OK：发送数据成功,RN_TCP_DISCONNECT：设备断开，RN_TCP_CH_DISCONNECT：通道连接，其他：发送失败
RJ_API int ndm_conn_send(net_dev_man_h handle, int dev_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief 连接接收数据
/// @param [in] handle          句柄
/// @param [out] *p_dev_id      返回设备ID
/// @param [out] *p_recv_data   缓冲区长度的指针
/// @return int， RN_TCP_OK：成功，其他：接收失败
RJ_API int ndm_conn_recv(net_dev_man_h handle, int *p_dev_id, rj_net_r_h *p_recv_data);

///释放接收的数据
/// @param [in] handle          句柄
/// @param [in] dev_id          标识设备
/// @param [in] recv_data       上次接收到的包
RJ_API void ndm_conn_free_mem(net_dev_man_h handle, int dev_id, rj_net_r_h recv_data);

#endif
//__NET_DEV_MAN_H__