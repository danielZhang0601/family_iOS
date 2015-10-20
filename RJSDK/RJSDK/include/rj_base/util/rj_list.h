///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/16
//
/// @file	 rj_list.h
/// @brief	 ˫���б���֧��ģ�壬��֧�ֶ��̰߳�ȫ
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_LIST_H__
#define __RJ_LIST_H__

#include "util/rj_type.h"

typedef struct rj_list_t rj_list_t;
typedef rj_list_t* rj_list_h;

typedef struct rj_list_node_t rj_list_node_t;

//////////////////////////////////////////////////////////////////////////
typedef rj_list_node_t * rj_iterator;

RJ_API rj_iterator rj_iter_add(rj_iterator iter);

RJ_API rj_iterator rj_iter_dec(rj_iterator iter);

RJ_API void * rj_iter_data(rj_iterator iter);
//////////////////////////////////////////////////////////////////////////

/// @brief ��ʼ���б�
/// @return rj_list_h      �����б�����������ʧ���򷵻�NULL��
RJ_API rj_list_h rj_list_create();

/// @brief �ͷ��б�
/// @param [in] *p_list      �б���
RJ_API void rj_list_destroy(rj_list_h handle);

/// @brief ����ڵ㵽ͷ��
/// @param [in] handle      �б���
/// @param [in] void * p_data  �����������ָ��
RJ_API void rj_list_push_front(rj_list_h handle, void *p_data);

/// @brief ����ڵ㵽β��
/// @param [in] handle      �б���
/// @param [in] void * p_data  �����������ָ��
RJ_API void rj_list_push_back(rj_list_h handle, void *p_data);

/// @brief ����ͷ���ڵ���û�����
/// @param [in] handle      �б���
/// @return void *      �����û�����ָ�룬�����򷵻�NULL��
RJ_API void * rj_list_front(rj_list_h handle);

/// @brief ����β���ڵ���û�����
/// @param [in] handle      �б���
/// @return void *      �����û�����ָ�룬�����򷵻�NULL��
RJ_API void * rj_list_back(rj_list_h handle);

/// @brief ����ͷ���ڵ���û�����
/// @param [in] handle      �б���
/// @return void *      �����û�����ָ�룬�����򷵻�NULL��
RJ_API void * rj_list_pop_front(rj_list_h handle);

/// @brief ����β���ڵ���û�����
/// @param [in] handle      �б���
/// @return void *      �����û�����ָ�룬�����򷵻�NULL��
RJ_API void * rj_list_pop_back(rj_list_h handle);

/// @brief �б�Ľڵ����
/// @param [in] handle      �б���
/// @return uint32      ���ؽڵ������
RJ_API uint32 rj_list_size(rj_list_h handle);

/// @brief �õ��б��������ͷ
/// @param [in] handle      �б���
/// @return rj_iterator      ��������
RJ_API rj_iterator rj_list_begin(rj_list_h handle);

/// @brief �õ��б��������β
/// @param [in] handle      �б���
/// @return rj_iterator      ��������
RJ_API rj_iterator rj_list_end(rj_list_h handle);

/// @brief ɾ��ĳ���ڵ�
/// @param [in] handle      �б���
/// @param [in] p_data      ����
RJ_API void rj_list_remove(rj_list_h handle, void *p_data);

/// @brief ɾ��ĳ���ڵ�
/// @param [in] handle      �б���
/// @param [in] iter        ������
RJ_API void rj_list_remove_iter(rj_list_h handle, rj_iterator iter);
#endif
