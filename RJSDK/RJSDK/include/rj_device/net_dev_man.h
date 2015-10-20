///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 5-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/21
//
/// @file	 net_dev_man.h
/// @brief	 �����������
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __NET_DEV_MAN_H__
#define __NET_DEV_MAN_H__

#include "util/rj_type.h"
#include "pub/rj_net_read.h"
#include "rj_net_pre_conn.h"

typedef struct uv_loop_s uv_loop_t;
typedef struct net_dev_man_t net_dev_man_t;
typedef net_dev_man_t* net_dev_man_h;

/// @brief ����һ�����������
/// @param [in] s_sbl   ���ͻ����ڴ��Ĵ�С
/// @param [in] sbl     ���ͻ����ܵĴ�С
/// @param [in] s_rbl   ���ջ����ڴ��Ĵ�С
/// @param [in] s_sbl   ���ͻ����ܵĴ�С
/// @return net_dev_man_h ���
RJ_API net_dev_man_h ndm_create(rn_server_h ser_handle,rn_client_h cli_handle,uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl);

/// @brief ����һ�����������
/// @param [in] handle ���
RJ_API void ndm_destroy(net_dev_man_h handle);

/// @brief ֹͣ����
/// @param [in] handle ���
RJ_API void ndm_stop(net_dev_man_h handle);

/// @brief ����һ���µ�����ͨ������Ԥ����ģ���У�
/// @param [in] handle ���
/// @param [in] p_addr �豸��IP��ַ
/// @param [in] port �豸�Ķ˿ں�
/// @param [out] p_ch_id ����ID
/// @return int RN_TCP_OK���ɹ���������ʧ��
RJ_API int ndm_create_ch(net_dev_man_h handle, char *p_addr,unsigned short port,int *p_ch_id);

/// @brief ʹ��һ���µ�����ͨ�������ǰ�����Ԥ����ͨ���Ƶ�һ��������ȥ�����ߴ���һ���µ����ӣ�
/// @param [in] handle      ���
/// @param [in] dev_id      �豸ID�����ⲿͳһ����
/// @param [in] ch_id       ����ͨ��ID�����ڲ�����
/// @return int RN_TCP_OK���ɹ���������ʧ��
RJ_API int ndm_enable_ch(net_dev_man_h handle, int dev_id, int ch_id);

/// @brief ��ʹ��һ������ͨ��
/// @param [in] handle ���
/// @param [in] dev_id      �豸ID�����ⲿͳһ����
/// @param [in] ch_id       ����ͨ��ID�����ڲ�����
RJ_API void ndm_disable_ch(net_dev_man_h handle, int dev_id, int ch_id);

/// @brief �ر�ĳ���豸
/// @param [in] handle ���
/// @param [in] dev_id      �豸ID�����ⲿͳһ����
RJ_API void ndm_close_device(net_dev_man_h handle, int dev_id);

/// @brief �ر�Ԥ����
/// @param [in] handle      ���
/// @param [in] ch_id       ����ͨ��ID
RJ_API void ndm_pcon_close_ch(net_dev_man_h handle, int ch_id);

/// @brief Ԥ���ӷ�������
/// @param [in] handle      ���
/// @param [in] ch_id       ͨ��ID
/// @param [in] p_ndp       ���ݰ�ͷ
/// @param [in] p_data      ���ݻ�����ָ��
/// @return int RN_TCP_OK�����ͳɹ���RN_TCP_DISCONNECT�����ӶϿ���������ʧ��
RJ_API int ndm_pconn_send(net_dev_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief Ԥ���ӽ�������
/// @param [in] handle      ���
/// @param [out] *p_ch_id   ͨ��ID
/// @param [int] *p_buf     ����ָ��
/// @param [in] buf_len     ����������
/// @param [out] p_data_len ���յ������ݳ���
/// @return int�� RN_TCP_OK���������ݳɹ�,RN_TCP_DISCONNECT�����ӶϿ���RN_TCP_NEW_CONNECT���������ӣ�������û����
RJ_API int ndm_pconn_recv(net_dev_man_h handle, int *p_ch_id, char *p_buf, uint32 buf_len, uint32 *p_data_len);

/// @brief ���ӷ���������������
/// @param [in] handle          ���
/// @param [in] dev_id          �����豸ID
/// @param [in] p_ndp           ���ݰ�ͷ
/// @param [in] p_data          ���ݻ�����ָ��
/// @return int�� RN_TCP_OK���������ݳɹ�,RN_TCP_DISCONNECT���豸�Ͽ���RN_TCP_CH_DISCONNECT��ͨ�����ӣ�����������ʧ��
RJ_API int ndm_conn_send(net_dev_man_h handle, int dev_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief ���ӽ�������
/// @param [in] handle          ���
/// @param [out] *p_dev_id      �����豸ID
/// @param [out] *p_recv_data   ���������ȵ�ָ��
/// @return int�� RN_TCP_OK���ɹ�������������ʧ��
RJ_API int ndm_conn_recv(net_dev_man_h handle, int *p_dev_id, rj_net_r_h *p_recv_data);

///�ͷŽ��յ�����
/// @param [in] handle          ���
/// @param [in] dev_id          ��ʶ�豸
/// @param [in] recv_data       �ϴν��յ��İ�
RJ_API void ndm_conn_free_mem(net_dev_man_h handle, int dev_id, rj_net_r_h recv_data);

#endif
//__NET_DEV_MAN_H__