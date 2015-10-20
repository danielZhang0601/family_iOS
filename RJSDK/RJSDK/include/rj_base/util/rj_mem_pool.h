///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_mem_pool.h
/// @brief	 �ṩ����̶���С���ڴ��
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_MEM_POOL__
#define __RJ_MEM_POOL__

#include "util/rj_type.h"
#include "pub/pub_type.h"

typedef struct rj_mem_pool_t rj_mem_pool_t;
typedef rj_mem_pool_t* rj_mem_pool_h;

/// @brief ��ʼ���ڴ��
/// @param [in] p_buf  �ڴ���׵�ַ
/// @param [in] block_num  �ڴ����Ŀ
/// @param [in] sub_block_len  �ڴ�鳤��
/// @return rj_mem_pool_h      �����ڴ�ؾ����������ʧ���򷵻�NULL��
RJ_API rj_mem_pool_h rj_mem_pool_create(char *p_buf, int block_num, int sub_block_len);

/// @brief �ͷ��ڴ�أ��ͷ�������ڴ棬ͬʱ�������еĽڵ���Ϣ��
/// @param [in] handle      �ڴ�ؾ��
RJ_API void rj_mem_pool_destroy(rj_mem_pool_h handle);

/// @brief �����ڴ��
/// @param [in] handle      �ڴ�ؾ��
/// @return _RJ_MEM_BLOCK_ *   �ڴ����Ϣ��ָ��
/// @note ������ rj_mem_pool_free �ͷŽڵ�
RJ_API _RJ_MEM_BLOCK_ * rj_mem_pool_malloc(rj_mem_pool_h handle);

/// @brief �ͷ��ڴ��
/// @param [in] handle      �ڴ�ؾ��
/// @param [in] p_block     �ڴ����Ϣ��ָ��
/// @note ������ rj_mem_pool_malloc ����
RJ_API void rj_mem_pool_free(rj_mem_pool_h handle, _RJ_MEM_BLOCK_ * p_block);
#endif
