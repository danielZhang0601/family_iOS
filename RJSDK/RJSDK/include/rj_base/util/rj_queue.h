///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/18
//
/// @file	 rj_queue.h
/// @brief	 ���У���֧��ģ�壬��֧�ֶ��̰߳�ȫ
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_QUEUE_H__
#define __RJ_QUEUE_H__

#include "util/rj_type.h"

typedef struct rj_queue_t rj_queue_t;
typedef rj_queue_t* rj_queue_h;

/// @brief ��ʼ���ж�
/// @return rj_queue_h      �����б�����������ʧ���򷵻�NULL��
RJ_API rj_queue_h rj_queue_create();

/// @brief �ͷŶ���
/// @param [in] handle      ���о��
RJ_API void rj_queue_destroy(rj_queue_h handle);

/// @brief ����ڵ㵽β��
/// @param [in] handle      ���о��
/// @param [in] void * p_data  �����������ָ��
RJ_API void rj_queue_push(rj_queue_h handle, void *p_data);

/// @brief ����ͷ���ڵ�
/// @param [in] handle      �б���
/// @param [out] void ** p_data  ����
/// @return int 0:�ɹ���1��ʧ��
RJ_API int rj_queue_pop(rj_queue_h handle, void **pp_data);

/// @brief ����ͷ���ڵ�
/// @param [in] handle      �б���
/// @return void *          ����ָ��
RJ_API void * rj_queue_pop_ret(rj_queue_h handle);

/// @brief ���еĽڵ����
/// @param [in] handle      ���о��
/// @return uint32      ���ؽڵ������
RJ_API uint32 rj_queue_size(rj_queue_h handle);
#endif
//__RJ_QUEUE_H__