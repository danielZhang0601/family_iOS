#include "rj_net_pre_conn.h"
#include "util/rj_list.h"
#include "pub/rj_ndp.h"
#include "sys/sys_pthread.h"
#include "util/logger.h"
#include "rn_tcp.h"
#include <assert.h>
#include <string.h>
#include <iostream>

const  uint32 MAX_PRE_CONN_MEM_LEN  = 1024;

///	@struct pre_conn_man_t 
///	@brief  Ԥ���ӹ���
/// @note �ṹ�������ֶζ����ڲ�ʹ��, �ⲿ����ֱ�ӷ���
typedef struct pconn_man_t
{
	rn_server_h			ser_handle;		/// �����������
	rn_client_h			cli_handle;		/// �ͻ������Ӷ���

	sys_mutex_t			*p_error_mutex;	/// ��ȥ�б�����
	rj_list_h			error_queue;	/// �����б�
	
	sys_mutex_t			*p_recv_mutex;	/// ��ȥ�б�����
	rj_list_h			recv_queue;     /// ���ն���

	sys_mutex_t			*p_conn_mutex;  /// �����б�����
    rj_list_h           conn_list;		/// �����б�

	sys_mutex_t			*p_close_mutex; /// ���ڹرյ������б���
    rj_list_h           close_list;     /// ���ڹرյ������б�
}pconn_man_t;

///	@struct rj_pcon_ch_t
///	@brief  ��������ͨ������ģ��
typedef struct rj_pcon_ch_t
{
    pconn_man_t		       *p_pconn;			///< �󶨵�ʹ�û�������(���磩

    rn_tcp_h				ws_conn;		    ///< ws����
    uv_buf_t                send_buf;			///< ���ͻ�������ַ
    uint32                  send_buf_len;       ///< ���ͻ���������
	uint32					sended;				///< ����Ƿ������rn_tcp_write
}rj_pcon_ch_t;

typedef struct rj_pcon_error
{
	int             ch_id;		///����ͨ��ID
	int				error_code;	///������
}rj_pcon_error;

typedef struct rj_pconn_recv_buffer
{
    int     ch_id;
	char    *pData;
	uint32  dataLen;
}rj_pconn_recv_buffer;


/// ��������ڴ�ص�����
static void pconn_tcp_cb_alloc(rn_tcp_h tcp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf)
{
	rj_pcon_ch_t *channl = (rj_pcon_ch_t *)(obj);
	assert(NULL != channl);
	assert(channl->ws_conn == tcp_h);

	//Ԥ����ģ��ֻ�ܴ���(����<-->�ظ�),���������յ��������ݰ������ʾ���ӳ���
	p_buf->base = new char[suggest_size + 32];
	p_buf->len = suggest_size;
}

/// ����ɻص�
static void pconn_tcp_cb_read(rn_tcp_h tcp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf)
{
	rj_pcon_ch_t *channl = (rj_pcon_ch_t *)(obj);
	assert(NULL != channl);
	assert(channl->ws_conn == tcp_h);
	/// nread >= 0 ��ʾ ������ȡ����ô������ ������ʾ��������error_list��֪ͨ�ϲ�
	if(nread > 0)
	{
		rj_pconn_recv_buffer *pBuff = new rj_pconn_recv_buffer;
		pBuff->pData = p_buf->base;
		pBuff->dataLen = nread;
		pBuff->ch_id = rn_tcp_tag(tcp_h);

		sys_mutex_lock(channl->p_pconn->p_recv_mutex);
		rj_list_push_back(channl->p_pconn->recv_queue,pBuff);
		sys_mutex_unlock(channl->p_pconn->p_recv_mutex);
	}
	else if(0 == nread)
	{
		if ((NULL != p_buf) && (NULL != p_buf->base))
		{
			delete [] p_buf->base;
		}
	}
	else
	{
        if ((NULL != p_buf) && (NULL != p_buf->base))
            delete [] p_buf->base;

		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = rn_tcp_tag(channl->ws_conn);
		p_error->error_code = RN_TCP_DISCONNECT;

		sys_mutex_lock(channl->p_pconn->p_error_mutex);
		rj_list_push_back(channl->p_pconn->error_queue,p_error);
		sys_mutex_unlock(channl->p_pconn->p_error_mutex);
	}
}

static void pre_tcp_cb_write(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret)
{
	rj_pcon_ch_t *channel = (rj_pcon_ch_t *)(obj);
	if((RN_TCP_OK == ret) || (RN_TCP_FREE == ret))
	{
		/// ��channel->sended�������ʾû�з������ݣ���channel->sended����1��ʾ�Ѿ������ˣ���
		/// pre_tcp_cb_write�ٴλص���������RN_TCP_OK == ret �� RN_TCP_FREE == ret˵�����ݷ���
		/// �������channel->sended����0
		if(0 == channel->sended)
		{
			if(channel->send_buf.len > 0)
			{
				rn_tcp_write(channel->ws_conn,&channel->send_buf);
				channel->sended = 1;
			}
		}
		else
		{
			channel->send_buf.len = 0;
			channel->sended = 0;
		}
	}
	else
	{
		/// ���ͳ���˵�����ӶϿ���֪ͨ�ϲ�
		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = rn_tcp_tag(channel->ws_conn);
		p_error->error_code = RN_TCP_DISCONNECT;

		sys_mutex_lock(channel->p_pconn->p_error_mutex);
		rj_list_push_back(channel->p_pconn->error_queue,p_error);
		sys_mutex_unlock(channel->p_pconn->p_error_mutex);
	}
}

static rj_pcon_ch_t * create_channel(pconn_man_t *p_pconn)
{
	rj_pcon_ch_t *channel = new rj_pcon_ch_t;
	memset(channel,0,sizeof(rj_pcon_ch_t));

	channel->p_pconn = p_pconn;
	channel->send_buf.base = new char[MAX_PRE_CONN_MEM_LEN];
	channel->send_buf.len = 0;
	channel->send_buf_len = MAX_PRE_CONN_MEM_LEN;
	channel->ws_conn = NULL;

	return channel;
}

static void destory_channel(rj_pcon_ch_t *channel)
{
	if(NULL == channel)
	{
		return;
	}

	delete [] channel->send_buf.base;
	delete channel;
}

static rj_pcon_ch_t * find_channel(rj_list_h conn_list, int ch_id)
{
	rj_pcon_ch_t *channel = NULL;
	rj_iterator iter = rj_list_begin(conn_list);
	rj_iterator end = rj_list_end(conn_list);

	while(iter != end)
	{
		if(rn_tcp_tag(((rj_pcon_ch_t *)(rj_iter_data(iter)))->ws_conn) == ch_id)
		{
			channel = (rj_pcon_ch_t *)(rj_iter_data(iter));
			break;
		}

		iter = rj_iter_add(iter);
	}

	return channel;
}
/// �ر���ɻص�
static void pre_tcp_cb_close(rn_tcp_h tcp_h, void *obj)
{
	pconn_man_t *pre_con_man = (pconn_man_t *)(obj);
	assert(pre_con_man != NULL);

	sys_mutex_lock(pre_con_man->p_close_mutex);
	rj_iterator iter = rj_list_begin(pre_con_man->close_list);
	while(iter != rj_list_end(pre_con_man->close_list))
	{
        rj_iterator temp = iter;
        iter = rj_iter_add(iter);

		rj_pcon_ch_t *channel = (rj_pcon_ch_t *)rj_iter_data(temp);
		if(channel->ws_conn == tcp_h)
		{
			destory_channel(channel);
            rj_list_remove(pre_con_man->close_list, temp);//��ɾ������Խ����Խ�࣬���Ҳ����˳�
			break;
		}
	}
	sys_mutex_unlock(pre_con_man->p_close_mutex);
}

/// ser�� ws���ֳɹ���ʧ�ܻص�
static void pre_tcp_ser_ws_cb_hs(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret)
{
	assert(NULL != tcp_h);
	assert(NULL != obj);
	rj_pcon_ch_t *channel = (rj_pcon_ch_t *)(obj);
	pconn_man_t *pre_con_man = channel->p_pconn;

	if(RN_TCP_OK == ret)
	{
		/// ֪ͨ�ϲ���������
		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = rn_tcp_tag(channel->ws_conn);
		p_error->error_code = RN_TCP_NEW_CONNECT;

		sys_mutex_lock(channel->p_pconn->p_error_mutex);
		rj_list_push_back(channel->p_pconn->error_queue,p_error);
		sys_mutex_unlock(channel->p_pconn->p_error_mutex);
	}
	else
	{
		//ser���յ��µ����ӣ���������ʧ�ܣ�ֱ�ӹرգ���conn_list�ƶ���close_list
		sys_mutex_lock(pre_con_man->p_conn_mutex);
		rj_list_remove(pre_con_man->conn_list,channel);
		sys_mutex_unlock(pre_con_man->p_conn_mutex);

		sys_mutex_lock(pre_con_man->p_close_mutex);
		rj_list_push_back(pre_con_man->close_list,channel);
		sys_mutex_unlock(pre_con_man->p_close_mutex);

		/// ����ʧ�ܣ�ֱ�ӹر�����
		rn_tcp_close(tcp_h,pre_tcp_cb_close,pre_con_man);
	}
}

/// cli�� ws���ֳɹ���ʧ�ܳɻص�
static void pre_tcp_cli_ws_cb_hs(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret)
{
	assert(NULL != tcp_h);
	assert(NULL != obj);
	rj_pcon_ch_t *channel = (rj_pcon_ch_t *)(obj);
	pconn_man_t *pre_con_man = channel->p_pconn;

	if(RN_TCP_OK == ret)
	{
		/// ֪ͨ�ϲ���������
		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = rn_tcp_tag(channel->ws_conn);
		p_error->error_code = RN_TCP_NEW_CONNECT;

		sys_mutex_lock(channel->p_pconn->p_error_mutex);
		rj_list_push_back(channel->p_pconn->error_queue,p_error);
		sys_mutex_unlock(channel->p_pconn->p_error_mutex);
	}
	else
	{
		/// ֪ͨ�ϲ�����ʧ��
		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = rn_tcp_tag(tcp_h);
		p_error->error_code = RN_TCP_DISCONNECT;

		sys_mutex_lock(pre_con_man->p_error_mutex);
		rj_list_push_back(pre_con_man->error_queue,p_error);
		sys_mutex_unlock(pre_con_man->p_error_mutex);
	}
}

/// ser�˽��յ���tcp���ӻص�
static int pre_tcp_cb_accept(rn_tcp_h tcp_h, void *obj)
{
	if (NULL == tcp_h)
	{
		return 0;
	}
	
	/// ������tcp���ӣ�ֱ�ӽ���ws����Э��
	pconn_man_t *pre_con_man = (pconn_man_t *)(obj);
	rj_pcon_ch_t *channel = create_channel(pre_con_man);
	channel->ws_conn = tcp_h;

	sys_mutex_lock(pre_con_man->p_conn_mutex);
	rj_list_push_back(pre_con_man->conn_list,channel);
	sys_mutex_unlock(pre_con_man->p_conn_mutex);

	/// �ȿ�ʼ��ȡ���ݣ��ڿ�ʼ����
	rn_tcp_read_start(tcp_h,pconn_tcp_cb_alloc,pconn_tcp_cb_read,channel);
	rn_tcp_ws_hs(tcp_h,(char *)RJ_WS_DEV_PROTOCOL,pre_tcp_ser_ws_cb_hs,channel);
	return 0;
}

/// cli�˽���tcp����ʧ�ܻ�ɹ��ص�
static void pre_tcp_cb_connect(rn_tcp_h tcp_h, void *obj,int tag, rn_tcp_ret_e ret)
{
	assert(NULL != obj);

	pconn_man_t *pre_con_man = (pconn_man_t *)(obj);
	
	if(RN_TCP_OK == ret)
	{
		/// ������tcp���ӣ�ֱ�ӽ���ws����Э��
		rj_pcon_ch_t *channel = create_channel(pre_con_man);
		channel->ws_conn = tcp_h;

		sys_mutex_lock(pre_con_man->p_conn_mutex);
		rj_list_push_back(pre_con_man->conn_list,channel);
		sys_mutex_unlock(pre_con_man->p_conn_mutex);

		/// �ȿ�ʼ��ȡ���ݣ��ڿ�ʼ����
		rn_tcp_read_start(tcp_h,pconn_tcp_cb_alloc,pconn_tcp_cb_read,channel);
		rn_tcp_ws_hs(tcp_h,(char *)RJ_WS_DEV_PROTOCOL,pre_tcp_cli_ws_cb_hs,channel);
	}
	else
	{
		/// ֪ͨ�ϲ�tcp����ʧ��
		rj_pcon_error *p_error = new rj_pcon_error;
		p_error->ch_id = tag;
		p_error->error_code = RN_TCP_DISCONNECT;

		sys_mutex_lock(pre_con_man->p_error_mutex);
		rj_list_push_back(pre_con_man->error_queue,p_error);
		sys_mutex_unlock(pre_con_man->p_error_mutex);
	}
}

RJ_API pcon_man_h pconn_man_create(rn_server_h ser_handle,rn_client_h cli_handle)
{
	if((NULL == ser_handle) || (NULL == cli_handle))
	{
		assert(false);
		return NULL;
	}

	pconn_man_t *pre_con_man = new pconn_man_t;
	memset(pre_con_man,0,sizeof(pconn_man_t));

	pre_con_man->cli_handle = cli_handle;
	pre_con_man->ser_handle = ser_handle;

	pre_con_man->close_list = rj_list_create();
	pre_con_man->conn_list = rj_list_create();
	pre_con_man->error_queue = rj_list_create();
	pre_con_man->recv_queue = rj_list_create();
	pre_con_man->p_conn_mutex = sys_mutex_create();
	pre_con_man->p_close_mutex = sys_mutex_create();
	pre_con_man->p_error_mutex = sys_mutex_create();
	pre_con_man->p_recv_mutex = sys_mutex_create();

	return pre_con_man;
}

RJ_API void pconn_man_destoy(pcon_man_h handle)
{
	if(NULL == handle)
	{
		assert(false);
		return;
	}

	pconn_man_t *pre_con_man = handle;
	handle->ser_handle = NULL;
	handle->cli_handle = NULL;
	
	while(rj_list_size(pre_con_man->error_queue))
	{
        //���ú����Ŀ�����һ����ʱ������
        rj_pcon_error * p_error = (rj_pcon_error *)rj_list_pop_front(pre_con_man->error_queue);
		delete p_error;
	}

	while(rj_list_size(pre_con_man->recv_queue))
	{
        rj_pconn_recv_buffer * p_buf = (rj_pconn_recv_buffer *)rj_list_pop_front(pre_con_man->recv_queue);
		delete [] p_buf->pData;
		delete p_buf;
	}

	rj_list_destroy(pre_con_man->recv_queue);
	rj_list_destroy(pre_con_man->error_queue);
	rj_list_destroy(pre_con_man->close_list);
	rj_list_destroy(pre_con_man->conn_list);

	sys_mutex_destroy(pre_con_man->p_close_mutex);
	sys_mutex_destroy(pre_con_man->p_conn_mutex);
	sys_mutex_destroy(pre_con_man->p_error_mutex);
	sys_mutex_destroy(pre_con_man->p_recv_mutex);

	delete pre_con_man;
}

RJ_API int pconn_man_start_listen(pcon_man_h handle, uint16 port)
{
	if(NULL == handle)
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	pconn_man_t *pre_con_man = handle;
	if(NULL == pre_con_man->ser_handle)
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	return rn_tcp_listen_start(pre_con_man->ser_handle,port,pre_tcp_cb_accept,pre_con_man);
}

RJ_API int pconn_man_stop_listen(pcon_man_h handle)
{
	if(NULL == handle)
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	pconn_man_t *pre_con_man = handle;
	rn_tcp_listen_stop(pre_con_man->ser_handle);

	return RN_TCP_OK;
}

RJ_API int pconn_connect(pcon_man_h handle, char *p_ip, uint16 port, int *p_ch_id)
{
	if((NULL == handle) || (NULL == p_ch_id) || (NULL == p_ip))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}
	
	pconn_man_t *pre_con_man = handle;
	int ret = rn_tcp_connect(pre_con_man->cli_handle,p_ip,port,pre_tcp_cb_connect,pre_con_man);
	if(RN_INVALID_TAG != ret)
	{
		*p_ch_id = ret;
		return RN_TCP_OK;
	}
	else
	{
		return RN_TCP_OTHER;
	}
}


RJ_API int pconn_send(pcon_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data)
{
	if((NULL == handle) || (NULL == p_ndp) || (NULL == p_data))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	rj_pcon_ch_t *channel = NULL;
	pconn_man_t *pre_con_man = handle;

	sys_mutex_lock(pre_con_man->p_conn_mutex);
	channel = find_channel(pre_con_man->conn_list,ch_id);

	if(NULL == channel)
	{
		sys_mutex_unlock(pre_con_man->p_conn_mutex);
		return RN_TCP_PARAM_ERR;
	}
	
	/// �ж������Ƿ�Ͽ�
	if(RN_CONNECTED != rn_tcp_state(channel->ws_conn))
	{
		sys_mutex_unlock(pre_con_man->p_conn_mutex);
		return RN_TCP_DISCONNECT;
	}

	/// �ж��Ƿ��ڷ�������
	if(channel->send_buf.len > 0)
	{
		sys_mutex_unlock(pre_con_man->p_conn_mutex);
		return RN_TCP_OTHER;
	}

	/// �жϷ��ͻ����������Ƿ񹻳�
	if(channel->send_buf_len < (p_ndp->pk_len + sizeof(rn_ws_packet_t)))
	{
		assert(false);
		sys_mutex_unlock(pre_con_man->p_conn_mutex);
		return RN_TCP_PARAM_ERR;
	}

	/// ������copy�����ͻ�����
	rn_ws_packet_t *pPackHead = (rn_ws_packet_t *)(channel->send_buf.base);
	memset(pPackHead,0,sizeof(rn_ws_packet_t));
	pPackHead->type = WS_TEXT;
	memcpy(channel->send_buf.base+sizeof(rn_ws_packet_t), p_data, p_ndp->pk_len);
    channel->send_buf.len = p_ndp->pk_len + sizeof(rn_ws_packet_t);
	channel->sended = 0;

	rn_tcp_try_write(channel->ws_conn, pre_tcp_cb_write, channel);

	sys_mutex_unlock(pre_con_man->p_conn_mutex);

	return RN_TCP_OK;
}

RJ_API int pconn_recv(pcon_man_h handle, int *p_ch_id, char *p_buf, uint32 buff_len,uint32 *p_data_len)
{
	if((NULL == handle) || (NULL == p_ch_id) || (NULL == p_buf) || (NULL == p_data_len) || (buff_len <= 0))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	int retValue;
	pconn_man_t *pre_con_man = handle;

	//��������������������ݣ��ȷ��ظ��ϲ�
	sys_mutex_lock(pre_con_man->p_error_mutex);
	if (rj_list_size(pre_con_man->error_queue) > 0)
	{
        rj_pcon_error *p_err = (rj_pcon_error *)rj_list_pop_front(pre_con_man->error_queue);
		retValue = p_err->error_code;
		*p_ch_id = p_err->ch_id;
		delete p_err;

		sys_mutex_unlock(pre_con_man->p_error_mutex);
		return retValue;
	}
	sys_mutex_unlock(pre_con_man->p_error_mutex);

	///���ƽ��յ������ݸ��ϲ�
	sys_mutex_lock(pre_con_man->p_recv_mutex);
	if(rj_list_size(pre_con_man->recv_queue) <= 0)
	{
		sys_mutex_unlock(pre_con_man->p_recv_mutex);
		return RN_TCP_OTHER;
	}

	rj_pconn_recv_buffer *pBuff = ((rj_pconn_recv_buffer *)rj_list_front(pre_con_man->recv_queue));
	
	/// �ж��û����Ļ����Ƿ��㹻
	if(pBuff->dataLen > buff_len)
	{
		sys_mutex_unlock(pre_con_man->p_recv_mutex);
		return RN_TCP_NO_ENOUGH;
	}

	/// �����ݿ������û����������ͷ��ڴ�
	*p_ch_id = pBuff->ch_id;
	*p_data_len = pBuff->dataLen;
	memcpy(p_buf,pBuff->pData,*p_data_len);
	delete [] pBuff->pData;
	delete pBuff;
	rj_list_pop_front(pre_con_man->recv_queue);

	sys_mutex_unlock(pre_con_man->p_recv_mutex);

	return RN_TCP_OK;
}

static void pconn_tcp_cb_read_stop(rn_tcp_h tcp_h, void *obj)
{
	rj_pcon_ch_t *channel = (rj_pcon_ch_t *)(obj);
	channel->p_pconn = NULL;
}

RJ_API int pconn_pop_ch(pcon_man_h handle, int ch_id, rn_tcp_h *p_ws_conn)
{
	if(NULL == handle)
	{
		return RN_TCP_PARAM_ERR;
	}

	rj_pcon_ch_t *channel = NULL;
	pconn_man_t *pre_con_man = handle;

	/// ��conn_list�������ch_id��Ӧ�����ӣ�����ҵ���conn_list�����Ƴ�
	sys_mutex_lock(pre_con_man->p_conn_mutex);
	channel = find_channel(pre_con_man->conn_list,ch_id);
	if(NULL != channel)
	{
		rj_list_remove(pre_con_man->conn_list,channel);
	}
	sys_mutex_unlock(pre_con_man->p_conn_mutex);

	/// �����ϲ�
	if(NULL != channel)
	{
		rn_tcp_read_stop(channel->ws_conn,pconn_tcp_cb_read_stop,channel);
		while(NULL != channel->p_pconn) sys_sleep(10);

		*p_ws_conn = channel->ws_conn;

		destory_channel(channel);
		return RN_TCP_OK;
	}

	return RN_TCP_OTHER;
}

/// �Ự���ɹ������Ե�������ӿڣ��ر�����
RJ_API void pconn_close_ch(pcon_man_h handle, int ch_id)
{
	if(NULL == handle)
	{
		return ;
	}

	rj_pcon_ch_t *channel = NULL;
	pconn_man_t *pre_con_man = handle;

	sys_mutex_lock(pre_con_man->p_conn_mutex);
	channel = find_channel(pre_con_man->conn_list,ch_id);
	if(NULL != channel)
	{
		rj_list_remove(pre_con_man->conn_list,channel);
	}
	sys_mutex_unlock(pre_con_man->p_conn_mutex);

	if(NULL != channel)
	{
		/// ��tcp������Ҫ����rn_tcp_close
		sys_mutex_lock(pre_con_man->p_close_mutex);
		rj_list_push_back(pre_con_man->close_list,channel);
		sys_mutex_unlock(pre_con_man->p_close_mutex);
		rn_tcp_close(channel->ws_conn,pre_tcp_cb_close,pre_con_man);
	}
}

/// �ر��������ӣ���ϵͳ�˳������������
RJ_API void pconn_close_all_connect(pcon_man_h handle)
{
	if(NULL == handle)
	{
		assert(false);
		return ;
	}

	pconn_man_t *pre_con_man = (pconn_man_t *)(handle);

	sys_mutex_lock(pre_con_man->p_conn_mutex);
	sys_mutex_lock(pre_con_man->p_close_mutex);
	rj_pcon_ch_t *channel = NULL;

	while(rj_list_size(pre_con_man->conn_list) > 0)
	{
		channel = (rj_pcon_ch_t *)(rj_list_pop_front(pre_con_man->conn_list));
		rj_list_push_back(pre_con_man->close_list,channel);
		rn_tcp_close(channel->ws_conn,pre_tcp_cb_close,pre_con_man);
	}
	sys_mutex_unlock(pre_con_man->p_close_mutex);
	sys_mutex_unlock(pre_con_man->p_conn_mutex);

	/// �ȴ�������Դ������
	bool b_close_compeleted = false;
	while(!b_close_compeleted)
	{
		if(0 == sys_mutex_trylock(pre_con_man->p_close_mutex))
		{
			b_close_compeleted = (rj_list_size(pre_con_man->close_list) <= 0);
			sys_mutex_unlock(pre_con_man->p_close_mutex);
		}
		sys_sleep(10);
	}
}

