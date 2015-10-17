#ifndef __RJ_NET_PRE_CONN_H__
#define __RJ_NET_PRE_CONN_H__

#include "uv.h"
#include "util/rj_type.h"
#include "rn_tcp.h"

typedef struct pconn_man_t pconn_man_t;
typedef pconn_man_t* pcon_man_h;

//////////////////////////////////////////////////////////////////////////
// functions

/// @brief ����Ԥ���ӹ���
/// @param [in] p_uv_loop  uv��Ϣѭ������
/// @return pcon_man_h     �����µ�Ԥ���ӹ�����, �������NULL, ��ʾʧ��
RJ_API pcon_man_h pconn_man_create(rn_server_h ser_handle,rn_client_h cli_handle);

/// @brief �ͷ�Ԥ���ӹ���
/// @param [in] p_conn_man  Ԥ���ӹ���
RJ_API void pconn_man_destoy(pcon_man_h handle);

/// @brief ����Ԥ���ӹ���ķ���ģʽ
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] port            �����˿ں�
/// @param [in] cb_accept       ���ӽ��ܻص�
/// @param [in] cb_obj          �ص�����
/// @return int ����RN_TCP_OK��ʾ�ɹ�, ����ֵ��ʾʧ��
RJ_API int pconn_man_start_listen(pcon_man_h handle, uint16 port);

/// @brief ֹͣԤ���ӹ���ķ���ģʽ
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] cb_stop         ��ֹͣ�Ļص�
/// @param [in] listen_port     �ص�����
/// @return int ����RN_TCP_OK��ʾ�ɹ�, ����ֵ��ʾʧ��
RJ_API int pconn_man_stop_listen(pcon_man_h handle);

/// @brief Ԥ����ͨ��, ����ɹ�, ��ʾ��������, ��Ҫ�ȴ� cb_connect �Ļص�
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] p_ip            �Է���ip
/// @param [in] port            �Է���port
/// @param [out] p_ch_id        ����id
/// @return int ����RN_TCP_OK��ʾ�ɹ�, ����ֵ��ʾʧ��
RJ_API int pconn_connect(pcon_man_h handle, char *p_ip, uint16 port, int *p_ch_id);

/// @brief ��������
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] ch_id           ����ͨ��ID
/// @param [in] p_ndp           ���ݰ�ͷ
/// @param [in] p_data          ���ݻ�����ָ��
/// @return int ����RN_TCP_OK���ͳɹ�,RN_TCP_DISCONNECT��ʾ���ӶϿ���������ʾʧ��
RJ_API int pconn_send(pcon_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data);

/// @brief ��������
/// @param [in] handle          Ԥ���Ӿ��
/// @param [out] p_ch_id        ����ͨ��ID, ���ڲ�����
/// @param [out] p_buf          ���ܻ�����ָ���ָ��
/// @param [out] p_data_len      ʵ���յ����ݳ���, �ڲ�����
/// @return int�� RN_TCP_OK���������ݳɹ�,RN_TCP_DISCONNECT�����ӶϿ���RN_TCP_NEW_CONNECT���������ӣ�������û����
RJ_API int pconn_recv(pcon_man_h handle, int *p_ch_id, char *p_buf,uint32 buff_len, uint32 *p_data_len);

/// @brief �Ƴ�Ԥ����(ͨ������)
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] ch_id           ����ͨ��ID
/// @param [out] pp_ws_conn      Ԥ���ӵľ������Ӷ���(WebSocket)
/// @return int RN_TCP_OK��pop�ɹ���������ʧ��
/// @note ����ѵ���pconn_close(), ��Ӧ�ٵ���pconn_pop_ch() (�᷵�ش���)
RJ_API int pconn_pop_ch(pcon_man_h handle, int ch_id, rn_tcp_h *p_ws_conn);

/// @brief �ر�Ԥ����
/// @param [in] handle          Ԥ���Ӿ��
/// @param [in] ch_id           ����ͨ��ID
/// @note ����ѵ���pconn_pop_ch(), ����Ҫ����pconn_close_ch()��
RJ_API void pconn_close_ch(pcon_man_h handle, int ch_id);

//�ر���������
/// @param [in] handle          Ԥ���Ӿ��
RJ_API void pconn_close_all_connect(pcon_man_h handle);

#endif // __RJ_NET_PRE_CONN_H__

