//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/29
//
/// @file	 rn_tcp.h
/// @brief	 ����libuv����ʱ����
///          
/// @author  YSW
/// @version 0.1
/// @history �޸���ʷ
/// @warning û�о���
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
    RN_TCP_OK                = 0,   ///< һ��OK
    RN_TCP_PARAM_ERR,               ///< ��������
    RN_TCP_HS_FAIL,                 ///< Websocket����ʧ��
    RN_TCP_DISCONNECT,              ///< ���ӶϿ�
    RN_TCP_TIMEOUT,                 ///< ��ʱ
    RN_TCP_FREE,                    ///< ���ڿ��У���д�����ݡ�
    RN_TCP_ILLEGAL,                 ///< �Ƿ�����
    RN_TCP_NEW_CONNECT,             ///< �½�����
    RN_TCP_CH_DISCONNECT,           ///< ����ͨ���Ͽ�
    RN_TCP_OUT_OF_MEMORY,           ///< �ڴ治��
    RN_TCP_NO_ENOUGH,               ///< �ڴ治��
    RN_TCP_OTHER                    ///< ��������
}rn_tcp_ret_e;

typedef enum rn_state_e
{
    RN_CLOSE		= 0,		///< ���ӹر�
    RN_PAUSE,                   ///< ��ͣ�У���������read_stop��
    RN_CONNECTED,				///< �����ϣ��������������͡���������
    RN_DISCONNECT               ///< ���ӶϿ����ȴ��ر�
}rn_state_e;


const char  WS_TEXT         = 0x01;
const char  WS_BINARY       = 0x02;
const char  WS_CMD          = 0x03;
//////////////////////////////////////////////////////////////////////////

/// @brief �����������ͻ�������������������Ӻ�Ļص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      tag�������Ψһ��ʶ
/// @param [in]      obj�����ñ����ӵĶ���
/// @return      ret���ο�rn_tcp_cb_err_em
typedef void (*tcp_cb_connect)(rn_tcp_h tcp_h, void *obj, int tag, rn_tcp_ret_e ret);

/// @brief �������������յ�һ����������Ļص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����ñ����ӵĶ���
/// @return 0��������������1����������������
typedef int (*tcp_cb_accept)(rn_tcp_h tcp_h, void *obj);

/// @brief �����������ر�һ�����ӵĻص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����ñ����ӵĶ���
typedef void (*tcp_cb_close)(rn_tcp_h tcp_h, void *obj);

/// @brief �������������ֵĻص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����ñ����ӵĶ���
/// @param [in]      ret���ο�rn_tcp_ret_e
typedef void (*tcp_ws_cb_hs)(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret);

/// @brief ���������������ӷ������ݵĻص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����ñ����ӵĶ���
/// @param [in]      ret���ο�rn_tcp_cb_err_em
typedef void (*tcp_cb_write)(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret);


/// @brief ����������ģ�����ϲ������ڴ� 
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����Ӷ����������
/// @param [in]      suggest_size: ��������ݳ���
/// @param [out]     p_buf��������ڴ��ַ��ָ��
typedef void (*tcp_cb_alloc)(rn_tcp_h tcp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf);


/// @brief ��������: ��ȡ�ص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����Ӷ����������
/// @param [in]      nread :��ȡ���ֽ�����С�����ʾ����
/// @param [out]     p_buf�����ݴ�ŵ���ʼ��ַ(�ײ㲻��ά����buf���ڴ��)
typedef void (*tcp_cb_read)(rn_tcp_h tcp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf);

/// @brief ��������: ��ȡֹͣ�ص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      obj�����Ӷ����������
typedef void (*tcp_cb_read_stop)(rn_tcp_h tcp_h, void *obj);
//////////////////////////////////////////////////////////////////////////

/// @brief ��������������libuv����
/// @param [in]      p_loop: libuv������
/// @return  rn_client_h ���ض�����
/// @note ������libuv����ǰ����
RJ_API rn_client_h rn_client_create(uv_loop_t  *p_loop);

/// @brief ��������������libuv����
/// @param [in]      loop��libuv������
/// @note ������libuv�رպ����
RJ_API void rn_client_destroy(rn_client_h loop);

/// @brief ��������������libuv����
/// @param [in]      p_loop: libuv������
/// @return  rn_server_h ���ض�����
/// @note ������libuv����ǰ����
RJ_API rn_server_h rn_server_create(uv_loop_t  *p_loop);

/// @brief ��������������libuv����
/// @param [in]      loop��libuv������
/// @note ������libuv�رպ����
RJ_API void rn_server_destroy(rn_server_h loop);

//////////////////////////////////////////////////////////////////////////

/// @brief ���������������˷�����������
/// @param [in]      loop��libuv������
/// @param [in]      p_ip:����˵�ip��ַ
/// @param [in]      port:����˵Ķ˿ں�
/// @param [in]      cb:���ӵĽ����ɹ���Ļص�����
/// @return ����tag����ΪRN_INVALID_TAG���ʾʧ��
RJ_API int rn_tcp_connect(rn_client_h loop, char *p_ip, uint16 port, tcp_cb_connect cb, void * cb_obj);

/// @brief �����������ر�һ������
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      cb :�ر����ӵĻص�����
/// @param [in]      cb_obj:�ص��������Ĳ���
RJ_API void rn_tcp_close(rn_tcp_h tcp_h, tcp_cb_close cb, void *cb_obj);

/// @brief �����������ر�һ������
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      cb :�ر����ӵĻص�����
/// @param [in]      cb_obj:�ص��������Ĳ���
/// @param [in]      s_timeout:��ʱʱ�䣨�룩
RJ_API void rn_tcp_set_timeout(rn_tcp_h tcp_h, int s_timeout);

/// @brief ������������������
/// @param [in]      loop��libuv������
/// @param [in]      port:����˵Ķ˿ں�
/// @param [in]      cb:�������ӵĻص�����
/// @return 0:�����ɹ���1������ʧ��
RJ_API int rn_tcp_listen_start(rn_server_h loop, uint16 port, tcp_cb_accept cb, void * cb_obj);

/// @brief �����������ر�����
/// @param [in]      loop��libuv������
RJ_API void rn_tcp_listen_stop(rn_server_h loop);

/// @brief ����������WS����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      p_protocol: ��Э��
/// @param [in]      cb :���ֵĻص�����
/// @param [in]      cb_obj:�ص��������Ĳ���
/// @return 0:�ɹ���1��ʧ�ܡ�
RJ_API int rn_tcp_ws_hs(rn_tcp_h tcp_h, char *p_protocol, tcp_ws_cb_hs cb, void *cb_obj);

//////////////////////////////////////////////////////////////////////////

/// @brief ��������������tag
/// @param [in]      tcp_h: ���Ӷ�����
/// @return int     ����Ψһ��ʶ
RJ_API int rn_tcp_tag(rn_tcp_h tcp_h);

/// @brief �������������Է���
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      cb :���͵Ļص�����
/// @param [in]      cb_obj:�ص��������Ĳ���
/// @return 0:�ɹ���1��ʧ�ܡ�
RJ_API int rn_tcp_try_write(rn_tcp_h tcp_h, tcp_cb_write cb, void *cb_obj);

/// @brief ������������������
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      p_data: ������ݵ�ָ�루uv_buf_t��
/// @param [in]      data_s: ���ݿ���Ŀ��Ҫ���0���򲻿ɼ��,��󲻳���64��
/// @return 0:�ɹ���1��ʧ�ܡ�
/// @note ���ӿ�ֻ����tcp_cb_try_write�Ļص�����tcp_cb_write����libuv�̵߳��ã����������̵߳��ã����򲻰�ȫ
/// ��ΪWebsocketģʽ������Ҫ�ڰ�ǰ����һ��rn_ws_packet_t��
RJ_API int rn_tcp_write(rn_tcp_h tcp_h, uv_buf_t * p_data);

/// @brief �������������ӽ����ɹ��󣬿�ʼ�����ݣ����ö�ȡ���ݺͽ������ݻ��������ڴ����Ļص�����
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      cb_alloc: �������ݻ��������ڴ����ص�����
/// @param [in]      cb_read:��ȡ���ݰ��Ļص�����
/// @param [in]      cb_obj:��ȡ���ݰ��ص�������������
/// @return ����ֵ  0���ɹ���1 :ʧ��
RJ_API int rn_tcp_read_start(rn_tcp_h tcp_h, tcp_cb_alloc cb_alloc, tcp_cb_read cb_read, void *cb_obj);


/// @brief �����������رն�ȡ����             
/// @param [in]      tcp_h: ���Ӷ�����
/// @param [in]      cb_read_stop:��ȡֹͣ�Ļص�����
/// @param [in]      cb_obj:��ȡ���ݰ��ص�������������
/// @note   ԭ���ϵ��øýӿڲ��ܸı�������ݵ��κ�״̬����ʱ�Ե�ֹͣ��������
RJ_API void rn_tcp_read_stop(rn_tcp_h tcp_h, tcp_cb_read_stop cb_read_stop, void *cb_obj);

/// @brief ��������������״̬             
/// @param [in]      tcp_h: ���Ӷ�����
RJ_API int rn_tcp_state(rn_tcp_h tcp_h);
#endif
//__RN_TCP_H__