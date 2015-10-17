//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/05
//
/// @file	 net_dev_discov.h
/// @brief	 网络设备发现
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __NET_DEV_DISCOV_H__
#define __NET_DEV_DISCOV_H__
#include "rn_udp.h"
#include "util/rj_queue.h"
#include "pub/pub_define.h"

typedef struct net_dev_discov_t net_dev_discov_t;
typedef net_dev_discov_t * dev_discov_h;

typedef struct discov_dev_t
{
	unsigned short port;
	char addr[PUB_IP_BUF];
	char hard_version[PUB_HW_VER_BUF];
	char sn[PUB_DEV_SN_BUF];
}discov_dev_t;

const char  DD_CLIENT       = 0x01;
const char  DD_IPC          = 0x02;
const char  DD_NVR          = 0x03;


//////////////////////////////////////////////////////////////////////////
/// @brief 函数描述：创建一个设备发现对象
/// @param [in]      udp_h: UDP的广播对象句柄
/// @param [in]      type:      类型
/// @return dev_discov_h 对象句柄
RJ_API dev_discov_h dev_discov_create(rn_udp_h udp_h, int type);

/// @brief 函数描述：销毁对象
/// @param [in]      discov_h: 对象句柄
RJ_API void dev_discov_destroy(dev_discov_h discov_h);

/// @brief 函数描述：修改地址+端口
/// @param [in]      discov_h:  对象句柄
/// @param [in]      ip:        IP地址
/// @param [in]      port:      端口
RJ_API void dev_discov_set_addr(dev_discov_h discov_h, char *ip, uint16 port);

/// @brief 函数描述：广播标题
/// @param [in]      discov_h:  对象句柄
/// @param [in]      port:      对方的端口
RJ_API void dev_discov_broadcast(dev_discov_h discov_h, uint16 port);

/// @brief 函数描述：回复标题
/// @param [in]      discov_h:  对象句柄
/// @param [in]      ip:        对方的IP地址
/// @param [in]      port:      对方的端口
RJ_API void dev_discov_reply(dev_discov_h discov_h, char *ip, uint16 port);

/// @brief 函数描述：获取设备列表
/// @param [in]      discov_h:  对象句柄
/// @param [out]     device:    空的设备列表句柄
RJ_API void dev_discov_device(dev_discov_h discov_h, rj_queue_h device);
#endif
//__NET_DEV_DISCOV_H__
