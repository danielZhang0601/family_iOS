///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/10
//
/// @file	 rj_key.h
/// @brief	 �������KEYö��
/// @author  YSW
/// @version 0.1
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_KEY_H__
#define __RJ_KEY_H__

#include "pub/rj_caption.h"

typedef enum rj_key_e
{
    // common
    RK_CMD                 = 0,
    RK_DATA,
    RK_CODE,
    RK_IP,
    RK_PORT,
    RK_DEV_ONLINE,
    RK_DEV_SN,
    RK_DEV_LICSENSE,
    RK_DEV_FW_VER,
    RK_DEV_UPNP_PORT,
    RK_AUTH_CODE,
    RK_EXPIRED_TIME,
    RK_NOTIFY_RS_RES,
    RK_RS_RES_ID
}rj_key_e;

/// @brief ���ļ���JSON���м���rj_key��ӳ���
/// @param [in] key_cap     ӳ���
/// @param [in] fn          ��Դ�ļ���
/// @return int ����0��ʾ�ɹ�������ֵ��ʾʧ��
int rj_key_load_res(rj_cap_h key_cap, const char* fn);

#endif//__RJ_KEY_H__
