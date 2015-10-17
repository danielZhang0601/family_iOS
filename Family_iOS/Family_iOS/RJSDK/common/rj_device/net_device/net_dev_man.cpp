#include "net_dev_man.h"
#include "util/rj_list.h"
#include "sys/sys_pthread.h"
#include "rj_net_pre_conn.h"
#include "rj_net_multi_conn.h"
#include <assert.h>
#include <string.h>

typedef struct net_dev_t
{
    int  dev_id;         ///< 设备ID       
	int  curr_ch_id;     ///< 当前使用的网络通道
}net_dev_t;

typedef struct net_dev_man_t
{
    pcon_man_h      pre_conn;
    rj_net_m_conn_h multi_conn;

    rj_list_h       dev_list;
	sys_mutex_t		*p_dev_lock;
}net_dev_man_t;


static net_dev_t* find_device(rj_list_h client_list, int dev_id)
{
	assert (NULL != client_list);

	net_dev_t *p_dev = NULL;
	rj_iterator iter = rj_list_begin(client_list);
	while (iter != rj_list_end(client_list))
	{
		p_dev = (net_dev_t *)rj_iter_data(iter);
		assert (NULL != p_dev);

		if (p_dev->dev_id == dev_id)
		{
			return p_dev;
		}

		iter =  rj_iter_add(iter);
	}

	return NULL;
}

RJ_API net_dev_man_h ndm_create(rn_server_h ser_handle,rn_client_h cli_handle,uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl)
{
	if((NULL == ser_handle) || (NULL == cli_handle))
	{
		assert(false);
		return NULL;
	}

	net_dev_man_t *p_dev_man = new net_dev_man_t;
	p_dev_man->dev_list = rj_list_create();
	p_dev_man->pre_conn = pconn_man_create(ser_handle,cli_handle);
	p_dev_man->multi_conn = rj_m_conn_create(s_sbl,sbl,s_rbl,rbl);
	p_dev_man->p_dev_lock = sys_mutex_create();

	assert(NULL != p_dev_man->pre_conn);
	assert(NULL != p_dev_man->multi_conn);
	assert(NULL != p_dev_man->p_dev_lock);
	assert(NULL != p_dev_man->dev_list);

	return p_dev_man;
}

RJ_API void ndm_destroy(net_dev_man_h handle)
{
	if(NULL == handle)
	{
		assert(false);
		return;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	rj_iterator begin = rj_list_begin(p_dev_man->dev_list);
	rj_iterator end = rj_list_end(p_dev_man->dev_list);
	while(begin != end)
	{
		delete (net_dev_t *)(rj_iter_data(begin));
		begin = rj_iter_add(begin);
	}
	rj_list_destroy(p_dev_man->dev_list);
	rj_m_conn_destroy(p_dev_man->multi_conn);
	pconn_man_destoy(p_dev_man->pre_conn);
	sys_mutex_destroy(p_dev_man->p_dev_lock);

	delete p_dev_man;
}

RJ_API void ndm_stop(net_dev_man_h handle)
{
	if(NULL == handle)
	{
		assert(false);
		return;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	rj_m_conn_stop(p_dev_man->multi_conn);
	pconn_close_all_connect(p_dev_man->pre_conn);
}

RJ_API int ndm_create_ch(net_dev_man_h handle, char *p_addr, unsigned short port, int *p_ch_id)
{
	if((NULL == handle) || (NULL == p_ch_id) || (NULL == p_addr))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	return pconn_connect(p_dev_man->pre_conn, p_addr, port, p_ch_id);
}

RJ_API int ndm_enable_ch(net_dev_man_h handle, int dev_id, int ch_id)
{
	if(NULL == handle)
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	rn_tcp_h ws_con = NULL;
	net_dev_t *p_dev = NULL;
	
	int ret_code = pconn_pop_ch(p_dev_man->pre_conn, ch_id, &ws_con);
	if(RN_TCP_OK != ret_code)
	{
		return ret_code;
	}
	assert(NULL != ws_con);

	sys_mutex_lock(p_dev_man->p_dev_lock);
	p_dev = find_device(p_dev_man->dev_list, dev_id);
	if(NULL == p_dev)
	{
		p_dev = new net_dev_t;
		assert(NULL != p_dev);
		memset(p_dev,0,sizeof(net_dev_t));
		p_dev->dev_id       = dev_id;
		p_dev->curr_ch_id   = ch_id;
		rj_list_push_back(p_dev_man->dev_list,p_dev);
	}

	rj_m_conn_push_ch(p_dev_man->multi_conn, p_dev->dev_id, ws_con);
	sys_mutex_unlock(p_dev_man->p_dev_lock);

	return ret_code;
}

RJ_API void ndm_disable_ch(net_dev_man_h handle, int dev_id, int ch_id)
{
	if(NULL == handle)
	{
		assert(false);
		return ;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	sys_mutex_lock(p_dev_man->p_dev_lock);
	net_dev_t *p_dev = find_device(p_dev_man->dev_list, dev_id);
	if (NULL == p_dev)
	{
		sys_mutex_unlock(p_dev_man->p_dev_lock);
		return;
	}

	rj_m_conn_pop_ch(p_dev_man->multi_conn,p_dev->dev_id, ch_id);
	sys_mutex_unlock(p_dev_man->p_dev_lock);
	
}

RJ_API void ndm_close_device(net_dev_man_h handle, int dev_id)
{
	if(NULL == handle)
	{
		assert(false);
		return ;
	}
	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	sys_mutex_lock(p_dev_man->p_dev_lock);
	net_dev_t *p_dev = find_device(p_dev_man->dev_list, dev_id);
	if (NULL == p_dev)
	{
		sys_mutex_unlock(p_dev_man->p_dev_lock);
		return;
	}

	rj_m_conn_stop_conn(p_dev_man->multi_conn, p_dev->dev_id);
	rj_list_remove(p_dev_man->dev_list,p_dev);
	delete p_dev;
	sys_mutex_unlock(p_dev_man->p_dev_lock);
}

//////////////////////////////////////////////////////////////////////////

RJ_API void ndm_pcon_close_ch(net_dev_man_h handle, int ch_id)
{
	if (NULL == handle)
	{
		assert(false);
		return ;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	pconn_close_ch(p_dev_man->pre_conn, ch_id);
}

RJ_API int ndm_pconn_send(net_dev_man_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data)
{
	if ((NULL == handle) || (NULL == p_ndp) || (NULL == p_data))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	return pconn_send(p_dev_man->pre_conn, ch_id, p_ndp, p_data);
}

RJ_API int ndm_pconn_recv(net_dev_man_h handle, int *p_ch_id, char *p_buf, uint32 buf_len,uint32 *p_data_len)
{
	if ((NULL == handle) || (NULL == p_ch_id) || (NULL == p_buf) || (NULL == p_data_len))
	{
		assert(false);
		return RN_TCP_PARAM_ERR;
	}

	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	return pconn_recv(p_dev_man->pre_conn, p_ch_id, p_buf, buf_len, p_data_len);
}
//////////////////////////////////////////////////////////////////////////

RJ_API int ndm_conn_send(net_dev_man_h handle, int dev_id, rj_ndp_pk_t *p_ndp, char *p_data)
{
	assert(NULL != handle);
	assert(NULL != p_ndp);
    assert(NULL != p_data);

    net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	if ((NULL != p_dev_man) && (NULL != p_ndp) && (NULL != p_data))
	{   
		net_dev_t *p_dev = find_device(p_dev_man->dev_list, dev_id);
		assert (NULL != p_dev);

		if (NULL != p_dev)
			return rj_m_conn_send(p_dev_man->multi_conn, p_dev->dev_id, p_dev->curr_ch_id, p_ndp, p_data);
	}

	return RN_TCP_PARAM_ERR;
}

RJ_API int ndm_conn_recv(net_dev_man_h handle, int *p_dev_id, rj_net_r_h *p_recv_data)
{
	net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
	assert (NULL != p_dev_man);
	assert (NULL != p_dev_id);
	assert (NULL != p_recv_data);

	if ((NULL != p_dev_man) && (NULL != p_dev_id) && (NULL != p_recv_data))
	{
		assert (NULL != p_dev_man->multi_conn);

		return rj_m_conn_recv(p_dev_man->multi_conn, p_dev_id, p_recv_data);
	}

	return RN_TCP_PARAM_ERR;
}

RJ_API void ndm_conn_free_mem(net_dev_man_h handle, int dev_id, rj_net_r_h recv_data)
{
    assert (NULL != handle);
	if (NULL != handle)
	{
        net_dev_man_t *p_dev_man = (net_dev_man_t *)(handle);
        rj_m_conn_free_mem(p_dev_man->multi_conn, dev_id, recv_data);
	}
}

