///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/17
//
/// @file	 rj_net_conn.h
/// @brief	 ���������������ض���
/// @author  YSW
/// @version 0.1
/// @warning û�о���
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

/// @brief ����һ������
/// @return rj_net_conn_h ���
RJ_API rj_net_conn_h rj_conn_create(rj_net_m_conn_h m_conn, uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);

/// @brief ����һ������
/// @param [in] handle ���
RJ_API void rj_conn_destroy(rj_net_conn_h handle);

/// @brief ��ȡ����ID
/// @param [in] handle ���
/// @return uint32 ����ID
RJ_API int rj_conn_id(rj_net_conn_h handle);


/// @brief ��������ID
/// @param [in] handle  ���
/// @param [in] id      idֵ
RJ_API void rj_conn_set_id(rj_net_conn_h handle, int id);


/// @brief ֹͣһ������
/// @param [in] handle ���
RJ_API void rj_conn_stop(rj_net_conn_h handle);

/// @brief ����һ������ͨ��
/// @param [in] handle ���
/// @param [in] tag ͨ��ID
/// @param [in] p_w_conn websocket�ľ��
/// @return 0���ɹ���1��ʧ��
RJ_API int rj_conn_push_ch(rj_net_conn_h handle, rn_tcp_h ws_tcp);

/// @brief ����һ������ͨ��
/// @param [in] handle ���
/// @param [in] ch_id ͨ��ID
RJ_API void rj_conn_pop_ch(rj_net_conn_h handle, int ch_id);

/// @brief ����һ�����ݣ��ж��RJ����
/// @param [in] handle ���
/// @param [in] ch_id      ָ��������ͨ��
/// @param [in] *p_ndp      NDP�����ͷ
/// @param [in] *p_data      ����ָ�루���ȼ�p_ndp��
/// @return int 0:�ɹ�����0,��������
RJ_API int rj_conn_send(rj_net_conn_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief ����һ������
/// @param [in] handle ���
/// @param [out] *p_recv_data ���ݻ�������ָ��
/// @return int 0:�ɹ���1��ʧ��
RJ_API int rj_conn_recv(rj_net_conn_h handle, rj_net_r_h *p_recv_data);

/// @brief �ͷ�һ���ڴ�
/// @param [in] handle ���
/// @param [out] recv_data ���ݻ�����
/// @return int 0:�ɹ���1��ʧ��
RJ_API void rj_conn_free_mem(rj_net_conn_h handle, rj_net_r_h recv_data);



#endif//__RJ_NET_CONN_H__
