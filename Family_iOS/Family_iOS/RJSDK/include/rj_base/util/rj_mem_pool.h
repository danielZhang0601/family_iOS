///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/15
//
/// @file	 rj_mem_pool.h
/// @brief	 提供多个固定大小的内存池
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_MEM_POOL__
#define __RJ_MEM_POOL__

#include "util/rj_type.h"
#include "pub/pub_type.h"

typedef struct rj_mem_pool_t rj_mem_pool_t;
typedef rj_mem_pool_t* rj_mem_pool_h;

/// @brief 初始化内存池
/// @param [in] p_buf  内存块首地址
/// @param [in] block_num  内存块数目
/// @param [in] sub_block_len  内存块长度
/// @return rj_mem_pool_h      返回内存池句柄，若创建失败则返回NULL。
RJ_API rj_mem_pool_h rj_mem_pool_create(char *p_buf, int block_num, int sub_block_len);

/// @brief 释放内存池（释放申请的内存，同时销毁所有的节点信息）
/// @param [in] handle      内存池句柄
RJ_API void rj_mem_pool_destroy(rj_mem_pool_h handle);

/// @brief 申请内存块
/// @param [in] handle      内存池句柄
/// @return _RJ_MEM_BLOCK_ *   内存块信息的指针
/// @note 必须由 rj_mem_pool_free 释放节点
RJ_API _RJ_MEM_BLOCK_ * rj_mem_pool_malloc(rj_mem_pool_h handle);

/// @brief 释放内存块
/// @param [in] handle      内存池句柄
/// @param [in] p_block     内存块信息的指针
/// @note 必须由 rj_mem_pool_malloc 申请
RJ_API void rj_mem_pool_free(rj_mem_pool_h handle, _RJ_MEM_BLOCK_ * p_block);
#endif
