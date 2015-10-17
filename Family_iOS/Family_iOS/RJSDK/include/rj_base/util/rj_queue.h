///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/18
//
/// @file	 rj_queue.h
/// @brief	 队列，不支持模板，不支持多线程安全
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_QUEUE_H__
#define __RJ_QUEUE_H__

#include "util/rj_type.h"

typedef struct rj_queue_t rj_queue_t;
typedef rj_queue_t* rj_queue_h;

/// @brief 初始化列队
/// @return rj_queue_h      返回列表句柄，若创建失败则返回NULL。
RJ_API rj_queue_h rj_queue_create();

/// @brief 释放队列
/// @param [in] handle      队列句柄
RJ_API void rj_queue_destroy(rj_queue_h handle);

/// @brief 插入节点到尾部
/// @param [in] handle      队列句柄
/// @param [in] void * p_data  待插入的数据指针
RJ_API void rj_queue_push(rj_queue_h handle, void *p_data);

/// @brief 弹出头部节点
/// @param [in] handle      列表句柄
/// @param [out] void ** p_data  数据
/// @return int 0:成功，1：失败
RJ_API int rj_queue_pop(rj_queue_h handle, void **pp_data);

/// @brief 弹出头部节点
/// @param [in] handle      列表句柄
/// @return void *          数据指针
RJ_API void * rj_queue_pop_ret(rj_queue_h handle);

/// @brief 队列的节点个数
/// @param [in] handle      队列句柄
/// @return uint32      返回节点个数。
RJ_API uint32 rj_queue_size(rj_queue_h handle);
#endif
//__RJ_QUEUE_H__