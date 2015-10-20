///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_net_write.h
/// @brief	 ������������ڴ�飨д��
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_WRITE_H__
#define __RJ_NET_WRITE_H__

#include "util/rj_type.h"
#include "pub/pub_type.h"
#include "pub/rj_ndp.h"


typedef _RJ_MEM_BLOCK_* rj_net_w_h;

/// @brief ��ʼ���ڴ��
/// @param [in] p_block  �ڴ����Ϣ
/// @return rj_net_w_h      ���ؾ����������ʧ���򷵻�NULL��
RJ_API rj_net_w_h rj_net_write_create(_RJ_MEM_BLOCK_ *p_block);

/// @brief ���پ��
/// @param [in] handle      ���
/// @return _RJ_MEM_BLOCK_ *      �ڴ����Ϣ
RJ_API _RJ_MEM_BLOCK_ * rj_net_write_destroy(rj_net_w_h handle);

/// @brief д��һ������
/// @param [in] handle      ���
/// @param [in] *p_ndp      NDP�����ͷ
/// @param [in] *p_data     ����ָ�루���ȼ�p_ndp��
/// @param [in] ws_type     WS���ͣ��������Websocket����Ϊ-1��
/// @return int     0��д��ɹ���1��ʧ�ܡ�
RJ_API int rj_net_write_push(rj_net_w_h handle, rj_ndp_pk_t *p_ndp, char *p_data, char ws_type);
#endif
//__RJ_NET_WRITE_H__