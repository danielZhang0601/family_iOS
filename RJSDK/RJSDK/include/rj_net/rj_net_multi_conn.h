///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/17
//
/// @file	 rj_net_multi_conn.h
/// @brief	 ��������������
/// @author  YSW
/// @version 0.1
/// @warning û�о���
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


/// @brief ����һ�������ӹ�����
/// @param [in] s_sbl       ���ͻ����ڴ��Ĵ�С
/// @param [in] sbl         ���ͻ����ܵĴ�С
/// @param [in] s_rbl       ���ջ����ڴ��Ĵ�С
/// @param [in] s_sbl       ���ͻ����ܵĴ�С
/// @return rj_net_m_conn_h ���
RJ_API rj_net_m_conn_h rj_m_conn_create(uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);


/// @brief ����һ�������ӹ�����
/// @param [in] handle ���
RJ_API void rj_m_conn_destroy(rj_net_m_conn_h handle);


/// @brief ֹͣ�����ӹ���
/// @param [in] handle ���
RJ_API void rj_m_conn_stop(rj_net_m_conn_h handle);


/// @brief ֹͣ����
/// @param [in] handle ���
/// @param [in] conn_id ����ID
RJ_API void rj_m_conn_stop_conn(rj_net_m_conn_h handle, int conn_id);


/// @brief ����һ������ͨ��
/// @param [in] handle  ���
/// @param [in] conn_id ����ID
/// @param [in] p_w_conn websocket�ľ��
/// @return 0:�ɹ�; 1:ʧ��, �����������д���ws_tcp
RJ_API int rj_m_conn_push_ch(rj_net_m_conn_h handle, int conn_id, rn_tcp_h ws_tcp);


/// @brief ����һ������ͨ��
/// @param [in] handle  ���
/// @param [in] conn_id ����ID
/// @param [in] ch_id   ͨ��ID
RJ_API void rj_m_conn_pop_ch(rj_net_m_conn_h handle, int conn_id, int ch_id);


/// @brief ����һ������(�ж��RJ��)
/// @param [in] handle      ���
/// @param [in] conn_id     ����ID
/// @param [in] net_ch_id   ͨ��ID
/// @param [in] *p_ndp      NDP�����ͷ
/// @param [in] *p_data     ����ָ��(���ȼ�p_ndp)
/// @return int 0:�ɹ�; ��0:����
RJ_API int rj_m_conn_send(rj_net_m_conn_h handle, int conn_id, int net_ch_id, rj_ndp_pk_t *p_ndp, char *p_data);


/// @brief ����һ������
/// @param [in] handle          ���
/// @param [out] *p_conn_id     ����ID��ָ��
/// @param [out] *p_recv_data   ���ݻ�������ָ��
/// @return int 0:�ɹ�; ��0,����
/// @note �����ȡ���ݲ���ʱ, �ᵼ�½��ջ��汻ռ��, ʱ����������TCP�Ͽ�
RJ_API int rj_m_conn_recv(rj_net_m_conn_h handle, int *p_conn_id, rj_net_r_h *p_recv_data);


/// @brief �ͷ�һ���ڴ�
/// @param [in] handle      ���
/// @param [in] conn_id     ����ID
/// @param [out] recv_data  ���ݻ�����
/// @return int 0:�ɹ�; 1:ʧ��
RJ_API void rj_m_conn_free_mem(rj_net_m_conn_h handle, int conn_id, rj_net_r_h recv_data);


#endif//__RJ_NET_MULTI_CONN_H__
