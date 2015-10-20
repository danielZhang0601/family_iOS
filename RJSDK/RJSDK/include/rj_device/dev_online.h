//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/10
//
/// @file	 dev_online.h
/// @brief	 �����豸����
///          
/// @author  YSW
/// @version 0.1
/// @history �޸���ʷ
/// @warning û�о���
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
    uint16  priority;           ///< ���ȼ�
    uint16  port;               ///< �˿�
    char    ip[PUB_IP_BUF];     ///< IP
}rj_dms_t;
//////////////////////////////////////////////////////////////////////////
/// @brief ������������ʼ��һ�����豸���ߡ�����
/// @param [in]      rn_client: ���ڲ������ӵĿͻ��˾��
/// @param [in]      p_dms:     DMS����Ϣ����ָ��
/// @param [in]      dms_num:   DMS����Ŀ
/// @return  dev_online_h ���ض�����
dev_online_h dev_online_init(rn_client_h rn_client, rj_dms_t *p_dms, int dms_num);

/// @brief ��������������ʼ��һ�����豸���ߡ�����
/// @param [in]      don_h:     ������
void dev_online_quit(dev_online_h don_h);

/// @brief ��������������UPNP�˿���Ϣ
/// @param [in]      don_h:     ������
/// @param [in]      port:      UPNP�Ķ˿�
/// @return          ��dev_online_ret
int dev_online_update(dev_online_h don_h, uint16 upnp_port);

/// @brief ������������ȡ���豸��Ȩ�롱
/// @param [in]      don_h:     ������
/// @param [out]     p_auth:    ���ڷ��ء��豸��Ȩ�롱��ָ��
/// @param [out]     buf_len:   ���ڷ��ء��豸��Ȩ�롱�Ļ���������
/// @return          ��dev_online_ret
int dev_online_auth_code(dev_online_h don_h, char *p_auth, int buf_len);

/// @brief ������������ȡ��ת����Դ��
/// @param [in]      don_h:     ������
/// @param [out]     p_rs_res:  ���ڷ��ء�ת����Դ����ָ��
/// @return          ��dev_online_ret
int dev_online_rs_id(dev_online_h don_h, rs_res_t *p_rs_res);

#endif
//__DEV_ONLINE_H__
