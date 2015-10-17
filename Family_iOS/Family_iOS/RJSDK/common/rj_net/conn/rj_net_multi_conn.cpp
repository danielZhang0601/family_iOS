#include "rj_net_multi_conn.h"
#include "rj_net_conn.h"
#include "util/rj_list.h"
#include "util/rj_queue.h"
#include "util/rj_mem_pool.h"
#include "pub/rj_net_read.h"
#include "pub/rj_net_write.h"
#include "sys/sys_mem.h"
#include "sys/sys_pthread.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

///	@struct rj_net_conn_t
///	@brief  多个客户端处理模块
typedef struct rj_net_multi_conn_t
{
    sys_mutex_t  *p_sys_mutex;          ///< 锁

    rj_iterator  curr_conn;              ///< 当前接收数据的连接
    rj_list_h    conn_list;              ///< 连接列表

    uint32       sub_send_buf_len;       ///< 发送缓存的块的大小
    uint32       send_buf_len;           ///< 发送缓存的总大小

    uint32       sub_recv_buf_len;       ///< 接收缓存的块的大小
    uint32       recv_buf_len;           ///< 接收缓存的总大小
}rj_net_multi_conn_t;

RJ_API rj_net_m_conn_h rj_m_conn_create(uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)sys_malloc(sizeof(rj_net_multi_conn_t));//new rj_net_multi_conn_t;
    assert (NULL != p_m_conn);

    p_m_conn->p_sys_mutex = sys_mutex_create();

    p_m_conn->conn_list = rj_list_create();
    assert (NULL != p_m_conn->conn_list);
    p_m_conn->curr_conn = rj_list_end(p_m_conn->conn_list);

    p_m_conn->sub_send_buf_len      = s_sbl;
    p_m_conn->send_buf_len          = sbl;
    p_m_conn->sub_recv_buf_len      = s_rbl;
    p_m_conn->recv_buf_len          = rbl;

    return (rj_net_m_conn_h)(p_m_conn);
}

RJ_API void rj_m_conn_destroy(rj_net_m_conn_h handle)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        while(0 < rj_list_size(p_m_conn->conn_list))
        {
            rj_net_conn_h net_conn = (rj_net_conn_h)(rj_list_pop_front(p_m_conn->conn_list));
            assert (NULL != net_conn);
            rj_conn_destroy(net_conn);
        }

        rj_list_destroy(p_m_conn->conn_list);

        sys_mutex_destroy(p_m_conn->p_sys_mutex);

        sys_free(p_m_conn);//delete p_m_conn;
    }
}

RJ_API void rj_m_conn_stop(rj_net_m_conn_h handle)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        while(0 < rj_list_size(p_m_conn->conn_list))
        {
            rj_net_conn_h net_conn = (rj_net_conn_h)rj_list_pop_front(p_m_conn->conn_list);

            rj_conn_stop(net_conn);
            rj_conn_destroy(net_conn);
        }

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }
}

rj_net_conn_h find_conn(rj_list_h conn_list, int conn_id)
{
    assert (NULL != conn_list);

    if (NULL != conn_list)
    {
        rj_iterator iter = rj_list_begin(conn_list);
        while (iter != rj_list_end(conn_list))
        {
            rj_net_conn_h net_conn = (rj_net_conn_h)rj_iter_data(iter);

            if (conn_id == rj_conn_id(net_conn))
            {
                return net_conn;
            }

            iter = rj_iter_add(iter);
        }
    }

    return NULL;
}

RJ_API void rj_m_conn_stop_conn(rj_net_m_conn_h handle, int conn_id)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        // 找到连接, 并关闭
        rj_net_conn_h net_conn = find_conn(p_m_conn->conn_list, conn_id);
        if (NULL != net_conn)
        {
            rj_list_remove(p_m_conn->conn_list, (void*)net_conn);
            
            rj_conn_stop(net_conn);
            rj_conn_destroy(net_conn);
        }

        // 简单处理: 直接将当前接收的连接置空
        // 下次接收数据的时候, 直接从连接列表头部开始
        p_m_conn->curr_conn = rj_list_end(p_m_conn->conn_list);

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }
}


RJ_API int rj_m_conn_push_ch(rj_net_m_conn_h handle, int conn_id, rn_tcp_h ws_tcp)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        rj_net_conn_h net_conn = find_conn(p_m_conn->conn_list, conn_id);
        if (NULL != net_conn)
        {
            // 加入已有的连接

            int ret = rj_conn_push_ch(net_conn, ws_tcp);

            sys_mutex_unlock(p_m_conn->p_sys_mutex);
            return ret;
        }
        else
        {
            // 加入一个新的连接

            rj_net_conn_h new_conn = rj_conn_create(p_m_conn, p_m_conn->sub_send_buf_len, p_m_conn->send_buf_len, p_m_conn->sub_recv_buf_len, p_m_conn->recv_buf_len);
            assert (NULL != new_conn);

            //设置id
            rj_conn_set_id(new_conn, conn_id);

            //将通道插入新的连接中
            if(0 != rj_conn_push_ch(new_conn, ws_tcp))
            {
                // 插入tcp失败
                rj_conn_destroy(new_conn);
                new_conn = NULL;

                sys_mutex_unlock(p_m_conn->p_sys_mutex);
                return 1;
            }

            //插入到列表尾部
            rj_list_push_back(p_m_conn->conn_list, new_conn);

            sys_mutex_unlock(p_m_conn->p_sys_mutex);
            return (NULL != new_conn) ? 0 : 1;
        }

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }

    return 1;
}


RJ_API void rj_m_conn_pop_ch(rj_net_m_conn_h handle, int conn_id, int ch_id)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        rj_net_conn_h net_conn = find_conn(p_m_conn->conn_list, conn_id);
        if (NULL != net_conn)
            rj_conn_pop_ch(net_conn, ch_id);
        //不可在此判断一个连接的通道为零, 把连接进行销毁, 由其他地方来处理真正的销毁.

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }
}


RJ_API int rj_m_conn_send(rj_net_m_conn_h handle, int conn_id, int net_ch_id, rj_ndp_pk_t *p_ndp, char *p_data)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);
    assert ((NULL != p_ndp) && (NULL != p_data));

    if ((NULL != p_m_conn) && (NULL != p_ndp) && (NULL != p_data))
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        rj_net_conn_h net_conn = find_conn(p_m_conn->conn_list, conn_id);
        if (NULL != net_conn)
        {
            int ret = rj_conn_send(net_conn, net_ch_id, p_ndp, p_data);
            sys_mutex_unlock(p_m_conn->p_sys_mutex);
            return ret;
        }
        else
        {
            // 没有找到通道信息
            // 调用者通过收发数据接口, 得到通道错误信息
            // 调用者再调用 rj_m_conn_stop_conn 关闭通道
            // 到这里, 说明是调用者关闭了通道, 还来发送数据, 这是调用者使用错误
            assert(false);
        }

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }

    assert(false);
    return RN_TCP_PARAM_ERR;
}


RJ_API int rj_m_conn_recv(rj_net_m_conn_h handle, int *p_conn_id, rj_net_r_h *p_recv_data)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);
    assert (NULL != p_conn_id);
    assert (NULL != p_recv_data);

    if ((NULL != p_m_conn) && (NULL != p_conn_id) && (NULL != p_recv_data))
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        //如果连接为0, 则直接退出
        if(rj_list_size(p_m_conn->conn_list) <= 0)
        {
            sys_mutex_unlock(p_m_conn->p_sys_mutex);
            return RN_TCP_OTHER;
        }

        //当前待接收的通道为空, 则取得一个连接
        if (rj_list_end(p_m_conn->conn_list) == p_m_conn->curr_conn)
        {
            assert (NULL != p_m_conn->conn_list);
            p_m_conn->curr_conn = rj_list_begin(p_m_conn->conn_list);
        }

        assert (rj_list_end(p_m_conn->conn_list) != p_m_conn->curr_conn);

        // 取到需要接收的通道后, 取这个通道的数据
        rj_net_conn_h net_conn = (rj_net_conn_h)rj_iter_data(p_m_conn->curr_conn);
        if (NULL != net_conn)
        {
            // 将接收通道, 移到下一个通道
            p_m_conn->curr_conn = rj_iter_add(p_m_conn->curr_conn);
            
            // 取得这个连接的id
            *p_conn_id = rj_conn_id(net_conn);

            // 接收这个通道的数据
            // 注意: 这个地方取得接收结果, 可能为通道已断开
            // 调用者后续需要调用 rj_m_conn_stop_conn 来关闭通道
            int ret = rj_conn_recv(net_conn, p_recv_data);
            sys_mutex_unlock(p_m_conn->p_sys_mutex);
            return ret;
        }
        else
        {
             // 内部逻辑错误
             assert(false);
        }

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }

    assert(false);
    return RN_TCP_PARAM_ERR;
}

RJ_API void rj_m_conn_free_mem(rj_net_m_conn_h handle, int conn_id, rj_net_r_h recv_data)
{
    rj_net_multi_conn_t *p_m_conn = (rj_net_multi_conn_t *)(handle);
    assert (NULL != p_m_conn);

    if (NULL != p_m_conn)
    {
        sys_mutex_lock(p_m_conn->p_sys_mutex);

        rj_net_conn_h net_conn = find_conn(p_m_conn->conn_list, conn_id);
        if (NULL != net_conn)
        {
            rj_conn_free_mem(net_conn, recv_data);
        }
        else
        {
            // 内部逻辑错误
            assert(false);
        }

        sys_mutex_unlock(p_m_conn->p_sys_mutex);
    }
}
//end
