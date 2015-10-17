//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/05
//
/// @file	 net_dev_discov.h
/// @brief	 �����豸����
///          
/// @author  YSW
/// @version 0.1
/// @history �޸���ʷ
/// @warning û�о���
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
/// @brief ��������������һ���豸���ֶ���
/// @param [in]      udp_h: UDP�Ĺ㲥������
/// @param [in]      type:      ����
/// @return dev_discov_h ������
RJ_API dev_discov_h dev_discov_create(rn_udp_h udp_h, int type);

/// @brief �������������ٶ���
/// @param [in]      discov_h: ������
RJ_API void dev_discov_destroy(dev_discov_h discov_h);

/// @brief �����������޸ĵ�ַ+�˿�
/// @param [in]      discov_h:  ������
/// @param [in]      ip:        IP��ַ
/// @param [in]      port:      �˿�
RJ_API void dev_discov_set_addr(dev_discov_h discov_h, char *ip, uint16 port);

/// @brief �����������㲥����
/// @param [in]      discov_h:  ������
/// @param [in]      port:      �Է��Ķ˿�
RJ_API void dev_discov_broadcast(dev_discov_h discov_h, uint16 port);

/// @brief �����������ظ�����
/// @param [in]      discov_h:  ������
/// @param [in]      ip:        �Է���IP��ַ
/// @param [in]      port:      �Է��Ķ˿�
RJ_API void dev_discov_reply(dev_discov_h discov_h, char *ip, uint16 port);

/// @brief ������������ȡ�豸�б�
/// @param [in]      discov_h:  ������
/// @param [out]     device:    �յ��豸�б���
RJ_API void dev_discov_device(dev_discov_h discov_h, rj_queue_h device);
#endif
//__NET_DEV_DISCOV_H__
