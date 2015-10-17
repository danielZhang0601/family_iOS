//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/05
//
/// @file	 rn_udp.h
/// @brief	 创建libuv运行时环境
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __RN_UDP_H__
#define __RN_UDP_H__
#include "util/rj_type.h"
#include "uv.h"

typedef struct rn_udp_t rn_udp_t;
typedef struct rn_udp_t * rn_udp_h;

typedef enum rn_udp_ret_e
{
    RN_UDP_OK                = 0,   ///< 一切OK
    RN_UDP_PARAM_ERR,               ///< 参数错误
    RN_UDP_TIMEOUT,                 ///< 超时
    RN_UDP_OTHER                    ///< 其他错误
}rn_udp_ret_e;

//////////////////////////////////////////////////////////////////////////


/// @brief 函数描述：对连接发送数据的回调函数
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      obj：调用本连接的对象
/// @param [in]      ret：参考rn_tcp_cb_err_em
typedef void (*udp_cb_send)(rn_udp_h udp_h, void *obj, rn_udp_ret_e ret);

/// @brief 函数描述：模块向上层申请内存 
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      obj：连接对象的上下文
/// @param [in]      suggest_size: 建议的数据长度
/// @param [out]     p_buf：分配的内存地址的指针
typedef void (*udp_cb_alloc)(rn_udp_h udp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf);


/// @brief 函数描述: 读取回调函数
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      obj：连接对象的上下文
/// @param [in]      nread :读取的字节数，小于零表示出错。
/// @param [in]      p_buf：数据存放的起始地址(底层不在维护此buf的内存块)
/// @param [in]      addr：对方的地址？
typedef void (*udp_cb_recv)(rn_udp_h udp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf, const struct sockaddr* addr);

//////////////////////////////////////////////////////////////////////////

/// @brief 函数描述：创建一个对象
/// @param [in]      p_loop: libuv对象句柄
/// @return rn_udp_h 对象句柄
RJ_API rn_udp_h rn_udp_create(uv_loop_t  *p_loop, uint16 port);

/// @brief 函数描述：销毁对象
/// @param [in]      udp_h: 连接对象句柄
RJ_API void rn_udp_destroy(rn_udp_h udp_h);

/// @brief 函数描述：尝试发送
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      cb :发送的回调函数
/// @param [in]      cb_obj:回调的上下文参数
/// @return 0:成功；1：失败。
RJ_API int rn_udp_try_write(rn_udp_h udp_h, udp_cb_send cb, void *cb_obj);

/// @brief 函数描述：尝试发送
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      addr: 网络地址
/// @param [in]      addr: 数据的指针（uv_buf_t）
/// @return 0:成功；1：失败。
/// @note 本接口由libuv线程调用，不能其他线程调用，否则不安全
RJ_API int rn_udp_send(rn_udp_h udp_h, struct sockaddr* addr, uv_buf_t * p_data);

/// @brief 函数描述：开始读数据，设置读取数据和接收数据缓冲区的内存分配的回调函数
/// @param [in]      udp_h: 连接对象句柄
/// @param [in]      cb_alloc: 接收数据缓冲区的内存分配回调函数
/// @param [in]      cb_read:读取数据包的回调函数
/// @param [in]      cb_obj:读取数据包回调函数的上下文
/// @return 返回值  0：成功，1 :失败
RJ_API int rn_udp_read_start(rn_udp_h udp_h, udp_cb_alloc cb_alloc, udp_cb_recv cb_read, void *cb_obj);


/// @brief 函数描述：关闭读取数据             
/// @param [in]      udp_h: 连接对象句柄
/// @note   原则上调用该接口不能改变接收数据的任何状态，暂时性的停止接收数据
RJ_API void rn_udp_read_stop(rn_udp_h udp_h);
#endif
//__RN_UDP_H__
