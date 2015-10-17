//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/05
//
/// @file	 rn_udp.h
/// @brief	 ����libuv����ʱ����
///          
/// @author  YSW
/// @version 0.1
/// @history �޸���ʷ
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __RN_UDP_H__
#define __RN_UDP_H__
#include "util/rj_type.h"
#include "uv.h"

typedef struct rn_udp_t rn_udp_t;
typedef struct rn_udp_t * rn_udp_h;

typedef enum rn_udp_ret_e
{
    RN_UDP_OK                = 0,   ///< һ��OK
    RN_UDP_PARAM_ERR,               ///< ��������
    RN_UDP_TIMEOUT,                 ///< ��ʱ
    RN_UDP_OTHER                    ///< ��������
}rn_udp_ret_e;

//////////////////////////////////////////////////////////////////////////


/// @brief ���������������ӷ������ݵĻص�����
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      obj�����ñ����ӵĶ���
/// @param [in]      ret���ο�rn_tcp_cb_err_em
typedef void (*udp_cb_send)(rn_udp_h udp_h, void *obj, rn_udp_ret_e ret);

/// @brief ����������ģ�����ϲ������ڴ� 
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      obj�����Ӷ����������
/// @param [in]      suggest_size: ��������ݳ���
/// @param [out]     p_buf��������ڴ��ַ��ָ��
typedef void (*udp_cb_alloc)(rn_udp_h udp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf);


/// @brief ��������: ��ȡ�ص�����
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      obj�����Ӷ����������
/// @param [in]      nread :��ȡ���ֽ�����С�����ʾ����
/// @param [in]      p_buf�����ݴ�ŵ���ʼ��ַ(�ײ㲻��ά����buf���ڴ��)
/// @param [in]      addr���Է��ĵ�ַ��
typedef void (*udp_cb_recv)(rn_udp_h udp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf, const struct sockaddr* addr);

//////////////////////////////////////////////////////////////////////////

/// @brief ��������������һ������
/// @param [in]      p_loop: libuv������
/// @return rn_udp_h ������
RJ_API rn_udp_h rn_udp_create(uv_loop_t  *p_loop, uint16 port);

/// @brief �������������ٶ���
/// @param [in]      udp_h: ���Ӷ�����
RJ_API void rn_udp_destroy(rn_udp_h udp_h);

/// @brief �������������Է���
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      cb :���͵Ļص�����
/// @param [in]      cb_obj:�ص��������Ĳ���
/// @return 0:�ɹ���1��ʧ�ܡ�
RJ_API int rn_udp_try_write(rn_udp_h udp_h, udp_cb_send cb, void *cb_obj);

/// @brief �������������Է���
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      addr: �����ַ
/// @param [in]      addr: ���ݵ�ָ�루uv_buf_t��
/// @return 0:�ɹ���1��ʧ�ܡ�
/// @note ���ӿ���libuv�̵߳��ã����������̵߳��ã����򲻰�ȫ
RJ_API int rn_udp_send(rn_udp_h udp_h, struct sockaddr* addr, uv_buf_t * p_data);

/// @brief ������������ʼ�����ݣ����ö�ȡ���ݺͽ������ݻ��������ڴ����Ļص�����
/// @param [in]      udp_h: ���Ӷ�����
/// @param [in]      cb_alloc: �������ݻ��������ڴ����ص�����
/// @param [in]      cb_read:��ȡ���ݰ��Ļص�����
/// @param [in]      cb_obj:��ȡ���ݰ��ص�������������
/// @return ����ֵ  0���ɹ���1 :ʧ��
RJ_API int rn_udp_read_start(rn_udp_h udp_h, udp_cb_alloc cb_alloc, udp_cb_recv cb_read, void *cb_obj);


/// @brief �����������رն�ȡ����             
/// @param [in]      udp_h: ���Ӷ�����
/// @note   ԭ���ϵ��øýӿڲ��ܸı�������ݵ��κ�״̬����ʱ�Ե�ֹͣ��������
RJ_API void rn_udp_read_stop(rn_udp_h udp_h);
#endif
//__RN_UDP_H__
