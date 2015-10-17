//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/29
//
/// @file	 rn_tcp.h
/// @brief	 创建libuv运行时环境
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __RN_TCP_H__
#define __RN_TCP_H__
#include "util/rj_type.h"
#include "pub/rj_ndp.h"
#include "uv.h"

//////////////////////////////////////////////////////////////////////////

typedef struct rn_tcp_t rn_tcp_t;
typedef struct rn_tcp_t * rn_tcp_h;

typedef struct rn_client_t rn_client_t;
typedef rn_client_t * rn_client_h;

typedef struct rn_server_t rn_server_t;
typedef rn_server_t * rn_server_h;

//////////////////////////////////////////////////////////////////////////

typedef enum rn_tcp_ret_e
{
    RN_TCP_OK                = 0,   ///< 一切OK
    RN_TCP_PARAM_ERR,               ///< 参数错误
    RN_TCP_HS_FAIL,                 ///< Websocket握手失败
    RN_TCP_DISCONNECT,              ///< 连接断开
    RN_TCP_TIMEOUT,                 ///< 超时
    RN_TCP_FREE,                    ///< 现在空闲，可写入数据。
    RN_TCP_ILLEGAL,                 ///< 非法访问
    RN_TCP_NEW_CONNECT,             ///< 新建连接
    RN_TCP_CH_DISCONNECT,           ///< 网络通道断开
    RN_TCP_OUT_OF_MEMORY,           ///< 内存不足
    RN_TCP_NO_ENOUGH,               ///< 内存不够
    RN_TCP_OTHER                    ///< 其他错误
}rn_tcp_ret_e;

typedef enum rn_state_e
{
    RN_CLOSE		= 0,		///< 连接关闭
    RN_PAUSE,                   ///< 暂停中，即调用了read_stop。
    RN_CONNECTED,				///< 连接上，本可以正常发送、接收数据
    RN_DISCONNECT               ///< 连接断开，等待关闭
}rn_state_e;


const char  WS_TEXT         = 0x01;
const char  WS_BINARY       = 0x02;
const char  WS_CMD          = 0x03;
//////////////////////////////////////////////////////////////////////////

/// @brief 函数描述：客户端向服务器发起建立连接后的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      tag：分配的唯一标识
/// @param [in]      obj：调用本连接的对象
/// @return      ret：参考rn_tcp_cb_err_em
typedef void (*tcp_cb_connect)(rn_tcp_h tcp_h, void *obj, int tag, rn_tcp_ret_e ret);

/// @brief 函数描述：接收到一个连接请求的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：调用本连接的对象
/// @return 0：接收连接请求；1：不接收连接请求
typedef int (*tcp_cb_accept)(rn_tcp_h tcp_h, void *obj);

/// @brief 函数描述：关闭一个连接的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：调用本连接的对象
typedef void (*tcp_cb_close)(rn_tcp_h tcp_h, void *obj);

/// @brief 函数描述：握手的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：调用本连接的对象
/// @param [in]      ret：参考rn_tcp_ret_e
typedef void (*tcp_ws_cb_hs)(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret);

/// @brief 函数描述：对连接发送数据的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：调用本连接的对象
/// @param [in]      ret：参考rn_tcp_cb_err_em
typedef void (*tcp_cb_write)(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret);


/// @brief 函数描述：模块向上层申请内存 
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：连接对象的上下文
/// @param [in]      suggest_size: 建议的数据长度
/// @param [out]     p_buf：分配的内存地址的指针
typedef void (*tcp_cb_alloc)(rn_tcp_h tcp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf);


/// @brief 函数描述: 读取回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：连接对象的上下文
/// @param [in]      nread :读取的字节数，小于零表示出错。
/// @param [out]     p_buf：数据存放的起始地址(底层不在维护此buf的内存块)
typedef void (*tcp_cb_read)(rn_tcp_h tcp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf);

/// @brief 函数描述: 读取停止回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      obj：连接对象的上下文
typedef void (*tcp_cb_read_stop)(rn_tcp_h tcp_h, void *obj);
//////////////////////////////////////////////////////////////////////////

/// @brief 函数描述：创建libuv对象
/// @param [in]      p_loop: libuv对象句柄
/// @return  rn_client_h 返回对象句柄
/// @note 必须在libuv启动前调用
RJ_API rn_client_h rn_client_create(uv_loop_t  *p_loop);

/// @brief 函数描述：创建libuv对象
/// @param [in]      loop：libuv对象句柄
/// @note 必须在libuv关闭后调用
RJ_API void rn_client_destroy(rn_client_h loop);

/// @brief 函数描述：创建libuv对象
/// @param [in]      p_loop: libuv对象句柄
/// @return  rn_server_h 返回对象句柄
/// @note 必须在libuv启动前调用
RJ_API rn_server_h rn_server_create(uv_loop_t  *p_loop);

/// @brief 函数描述：创建libuv对象
/// @param [in]      loop：libuv对象句柄
/// @note 必须在libuv关闭后调用
RJ_API void rn_server_destroy(rn_server_h loop);

//////////////////////////////////////////////////////////////////////////

/// @brief 函数描述：向服务端发起连接请求
/// @param [in]      loop：libuv对象句柄
/// @param [in]      p_ip:服务端的ip地址
/// @param [in]      port:服务端的端口号
/// @param [in]      cb:连接的建立成功后的回调函数
/// @return 返回tag，如为RN_INVALID_TAG则表示失败
RJ_API int rn_tcp_connect(rn_client_h loop, char *p_ip, uint16 port, tcp_cb_connect cb, void * cb_obj);

/// @brief 函数描述：关闭一个连接
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      cb :关闭连接的回调函数
/// @param [in]      cb_obj:回调的上下文参数
RJ_API void rn_tcp_close(rn_tcp_h tcp_h, tcp_cb_close cb, void *cb_obj);

/// @brief 函数描述：关闭一个连接
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      cb :关闭连接的回调函数
/// @param [in]      cb_obj:回调的上下文参数
/// @param [in]      s_timeout:超时时间（秒）
RJ_API void rn_tcp_set_timeout(rn_tcp_h tcp_h, int s_timeout);

/// @brief 函数描述：开启侦听
/// @param [in]      loop：libuv对象句柄
/// @param [in]      port:服务端的端口号
/// @param [in]      cb:接收连接的回调函数
/// @return 0:开启成功；1：开启失败
RJ_API int rn_tcp_listen_start(rn_server_h loop, uint16 port, tcp_cb_accept cb, void * cb_obj);

/// @brief 函数描述：关闭侦听
/// @param [in]      loop：libuv对象句柄
RJ_API void rn_tcp_listen_stop(rn_server_h loop);

/// @brief 函数描述：WS握手
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      p_protocol: 子协议
/// @param [in]      cb :握手的回调函数
/// @param [in]      cb_obj:回调的上下文参数
/// @return 0:成功；1：失败。
RJ_API int rn_tcp_ws_hs(rn_tcp_h tcp_h, char *p_protocol, tcp_ws_cb_hs cb, void *cb_obj);

//////////////////////////////////////////////////////////////////////////

/// @brief 函数描述：设置tag
/// @param [in]      tcp_h: 连接对象句柄
/// @return int     返回唯一标识
RJ_API int rn_tcp_tag(rn_tcp_h tcp_h);

/// @brief 函数描述：尝试发送
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      cb :发送的回调函数
/// @param [in]      cb_obj:回调的上下文参数
/// @return 0:成功；1：失败。
RJ_API int rn_tcp_try_write(rn_tcp_h tcp_h, tcp_cb_write cb, void *cb_obj);

/// @brief 函数描述：发送数据
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      p_data: 多块数据的指针（uv_buf_t）
/// @param [in]      data_s: 数据块数目（要求从0依序不可间断,最大不超过64）
/// @return 0:成功；1：失败。
/// @note 本接口只能在tcp_cb_try_write的回调函数tcp_cb_write中由libuv线程调用，不能其他线程调用，否则不安全
/// 若为Websocket模式，则需要在包前面置一个rn_ws_packet_t。
RJ_API int rn_tcp_write(rn_tcp_h tcp_h, uv_buf_t * p_data);

/// @brief 函数描述：连接建立成功后，开始读数据，设置读取数据和接收数据缓冲区的内存分配的回调函数
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      cb_alloc: 接收数据缓冲区的内存分配回调函数
/// @param [in]      cb_read:读取数据包的回调函数
/// @param [in]      cb_obj:读取数据包回调函数的上下文
/// @return 返回值  0：成功，1 :失败
RJ_API int rn_tcp_read_start(rn_tcp_h tcp_h, tcp_cb_alloc cb_alloc, tcp_cb_read cb_read, void *cb_obj);


/// @brief 函数描述：关闭读取数据             
/// @param [in]      tcp_h: 连接对象句柄
/// @param [in]      cb_read_stop:读取停止的回调函数
/// @param [in]      cb_obj:读取数据包回调函数的上下文
/// @note   原则上调用该接口不能改变接收数据的任何状态，暂时性的停止接收数据
RJ_API void rn_tcp_read_stop(rn_tcp_h tcp_h, tcp_cb_read_stop cb_read_stop, void *cb_obj);

/// @brief 函数描述：连接状态             
/// @param [in]      tcp_h: 连接对象句柄
RJ_API int rn_tcp_state(rn_tcp_h tcp_h);
#endif
//__RN_TCP_H__