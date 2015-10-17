#ifndef __RJ_NET_PRE_CONN_H__
#define __RJ_NET_PRE_CONN_H__

#include "uv.h"
#include "util/rj_type.h"
#include "rn_tcp.h"

typedef struct pconn_man_t pconn_man_t;
typedef pconn_man_t* pcon_man_h;

//////////////////////////////////////////////////////////////////////////
// functions

/// @brief 创建预连接管理
/// @param [in] p_uv_loop  uv消息循环对象
/// @return pcon_man_h     创建新的预连接管理句柄, 如果返回NULL, 表示失败
RJ_API pcon_man_h pconn_man_create(rn_server_h ser_handle,rn_client_h cli_handle);

/// @brief 释放预连接管理
/// @param [in] p_conn_man  预连接管理
RJ_API void pconn_man_destoy(pcon_man_h handle);

/// @brief 开启预连接管理的服务模式
/// @param [in] handle          预连接句柄
/// @param [in] port            侦听端口号
/// @param [in] cb_accept       连接接受回调
/// @param [in] cb_obj          回调对象
/// @return int 返回RN_TCP_OK表示成功, 其它值表示失败
RJ_API int pconn_man_start_listen(pcon_man_h handle, uint16 port);

/// @brief 停止预连接管理的服务模式
/// @param [in] handle          预连接句柄
/// @param [in] cb_stop         已停止的回调
/// @param [in] listen_port     回调对象
/// @return int 返回RN_TCP_OK表示成功, 其它值表示失败
RJ_API int pconn_man_stop_listen(pcon_man_h handle);

/// @brief 预连接通道, 如果成功, 表示正在连接, 需要等待 cb_connect 的回调
/// @param [in] handle          预连接句柄
/// @param [in] p_ip            对方的ip
/// @param [in] port            对方的port
/// @param [out] p_ch_id        连接id
/// @return int 返回RN_TCP_OK表示成功, 其它值表示失败
RJ_API int pconn_connect(pcon_man_h handle, char *p_ip, uint16 port, int *p_ch_id);

/// @brief 发送数据
/// @param [in] handle          预连接句柄
/// @param [in] ch_id           网络通道ID
/// @param [in] p_ndp           数据包头
/// @param [in] p_data          数据缓冲区指针
/// @return int 返回RN_TCP_OK发送成功,RN_TCP_DISCONNECT表示连接断开，其他表示失败
RJ_API int pconn_send(pcon_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief 接收数据
/// @param [in] handle          预连接句柄
/// @param [out] p_ch_id        网络通道ID, 由内部返回
/// @param [out] p_buf          接受缓冲区指针的指针
/// @param [out] p_data_len      实际收到数据长度, 内部返回
/// @return int， RN_TCP_OK：接收数据成功,RN_TCP_DISCONNECT：连接断开，RN_TCP_NEW_CONNECT：有新连接，其他：没数据
RJ_API int pconn_recv(pcon_man_h handle, int *p_ch_id, char *p_buf,uint32 buff_len, uint32 *p_data_len);

/// @brief 移出预连接(通道连接)
/// @param [in] handle          预连接句柄
/// @param [in] ch_id           网络通道ID
/// @param [out] pp_ws_conn      预连接的具体连接对象(WebSocket)
/// @return int RN_TCP_OK：pop成功，其他：失败
/// @note 如果已调用pconn_close(), 则不应再调用pconn_pop_ch() (会返回错误)
RJ_API int pconn_pop_ch(pcon_man_h handle, int ch_id, rn_tcp_h *p_ws_conn);

/// @brief 关闭预连接
/// @param [in] handle          预连接句柄
/// @param [in] ch_id           网络通道ID
/// @note 如果已调用pconn_pop_ch(), 则不需要调用pconn_close_ch()了
RJ_API void pconn_close_ch(pcon_man_h handle, int ch_id);

//关闭所有连接
/// @param [in] handle          预连接句柄
RJ_API void pconn_close_all_connect(pcon_man_h handle);

#endif // __RJ_NET_PRE_CONN_H__

