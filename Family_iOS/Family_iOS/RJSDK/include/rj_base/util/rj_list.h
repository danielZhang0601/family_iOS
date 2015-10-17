///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/16
//
/// @file	 rj_list.h
/// @brief	 双向列表，不支持模板，不支持多线程安全
/// @author  YSW
/// @version 0.1
/// @warning 没有警告
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

/// @brief 初始化列表
/// @return rj_list_h      返回列表句柄，若创建失败则返回NULL。
RJ_API rj_list_h rj_list_create();

/// @brief 释放列表
/// @param [in] *p_list      列表句柄
RJ_API void rj_list_destroy(rj_list_h handle);

/// @brief 插入节点到头部
/// @param [in] handle      列表句柄
/// @param [in] void * p_data  待插入的数据指针
RJ_API void rj_list_push_front(rj_list_h handle, void *p_data);

/// @brief 插入节点到尾部
/// @param [in] handle      列表句柄
/// @param [in] void * p_data  待插入的数据指针
RJ_API void rj_list_push_back(rj_list_h handle, void *p_data);

/// @brief 引用头部节点的用户数据
/// @param [in] handle      列表句柄
/// @return void *      返回用户数据指针，若无则返回NULL。
RJ_API void * rj_list_front(rj_list_h handle);

/// @brief 引用尾部节点的用户数据
/// @param [in] handle      列表句柄
/// @return void *      返回用户数据指针，若无则返回NULL。
RJ_API void * rj_list_back(rj_list_h handle);

/// @brief 弹出头部节点的用户数据
/// @param [in] handle      列表句柄
/// @return void *      返回用户数据指针，若无则返回NULL。
RJ_API void * rj_list_pop_front(rj_list_h handle);

/// @brief 弹出尾部节点的用户数据
/// @param [in] handle      列表句柄
/// @return void *      返回用户数据指针，若无则返回NULL。
RJ_API void * rj_list_pop_back(rj_list_h handle);

/// @brief 列表的节点个数
/// @param [in] handle      列表句柄
/// @return uint32      返回节点个数。
RJ_API uint32 rj_list_size(rj_list_h handle);

/// @brief 得到列表迭代器的头
/// @param [in] handle      列表句柄
/// @return rj_iterator      迭代器。
RJ_API rj_iterator rj_list_begin(rj_list_h handle);

/// @brief 得到列表迭代器的尾
/// @param [in] handle      列表句柄
/// @return rj_iterator      迭代器。
RJ_API rj_iterator rj_list_end(rj_list_h handle);

/// @brief 删除某个节点
/// @param [in] handle      列表句柄
/// @param [in] p_data      数据
RJ_API void rj_list_remove(rj_list_h handle, void *p_data);

/// @brief 删除某个节点
/// @param [in] handle      列表句柄
/// @param [in] iter        迭代器
RJ_API void rj_list_remove_iter(rj_list_h handle, rj_iterator iter);
#endif
