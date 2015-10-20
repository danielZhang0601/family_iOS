///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_net_read.h
/// @brief	 ������������ڴ�飨����
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NET_READ_H__
#define __RJ_NET_READ_H__

#include "util/rj_type.h"
#include "pub/pub_type.h"

typedef struct rj_net_read_t rj_net_read_t;
typedef rj_net_read_t* rj_net_r_h;

/// @brief ��ʼ���ڴ��
/// @param [in] p_block  �ڴ����Ϣ
/// @return rj_net_r_h      ���ؾ����������ʧ���򷵻�NULL��
RJ_API rj_net_r_h rj_net_read_create(_RJ_MEM_BLOCK_ *p_block, int ws);

/// @brief ���پ��
/// @param [in] handle      ���
/// @return _RJ_MEM_BLOCK_ *      �ڴ����Ϣ
RJ_API _RJ_MEM_BLOCK_ * rj_net_read_destroy(rj_net_r_h handle);

/// @brief ��ȡһ������
/// @param [in] handle      ���
/// @param [in] *p_data      ����
/// @return uint32     ���ݳ���
RJ_API uint32 rj_net_read_pop(rj_net_r_h handle, char **pp_data);
#endif
//__RJ_NET_READ_H__