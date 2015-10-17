//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/10
//
/// @file	 dev_online.h
/// @brief	 网络设备在线
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __DEV_ONLINE_H__
#define __DEV_ONLINE_H__
#include "util/rj_type.h"
#include "util/rj_queue.h"
#include "pub/pub_define.h"
#include "rn_tcp.h"

typedef struct dev_online_t dev_online_t;
typedef dev_online_t * dev_online_h;

typedef enum dev_online_ret
{
    DEV_RET_OK     = 0,
    DEV_RET_PARAM_ERR,
    DEV_RET_OFFLINE,
    DEV_RET_NO_RES
}dev_online_ret;

typedef struct rs_res_t
{
    uint16      enable;
    uint16      port;
    char        ip[PUB_IP_BUF];
    char        res_id[PUB_RS_RES_CODE_BUF];
}rs_res_t;

typedef struct rj_dms_t
{
    uint16  priority;           ///< 优先级
    uint16  port;               ///< 端口
    char    ip[PUB_IP_BUF];     ///< IP
}rj_dms_t;
//////////////////////////////////////////////////////////////////////////
/// @brief 函数描述：初始化一个“设备在线”对象
/// @param [in]      rn_client: 用于产生连接的客户端句柄
/// @param [in]      p_dms:     DMS的信息数组指针
/// @param [in]      dms_num:   DMS的数目
/// @return  dev_online_h 返回对象句柄
dev_online_h dev_online_init(rn_client_h rn_client, rj_dms_t *p_dms, int dms_num);

/// @brief 函数描述：反初始化一个“设备在线”对象
/// @param [in]      don_h:     对象句柄
void dev_online_quit(dev_online_h don_h);

/// @brief 函数描述：更新UPNP端口信息
/// @param [in]      don_h:     对象句柄
/// @param [in]      port:      UPNP的端口
/// @return          见dev_online_ret
int dev_online_update(dev_online_h don_h, uint16 upnp_port);

/// @brief 函数描述：获取“设备授权码”
/// @param [in]      don_h:     对象句柄
/// @param [out]     p_auth:    用于返回“设备授权码”的指针
/// @param [out]     buf_len:   用于返回“设备授权码”的缓冲区长度
/// @return          见dev_online_ret
int dev_online_auth_code(dev_online_h don_h, char *p_auth, int buf_len);

/// @brief 函数描述：获取“转发资源”
/// @param [in]      don_h:     对象句柄
/// @param [out]     p_rs_res:  用于返回“转发资源”的指针
/// @return          见dev_online_ret
int dev_online_rs_id(dev_online_h don_h, rs_res_t *p_rs_res);

#endif
//__DEV_ONLINE_H__
