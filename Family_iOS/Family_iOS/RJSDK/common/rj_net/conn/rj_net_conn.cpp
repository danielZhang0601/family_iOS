#include "rj_net_conn.h"
#include "util/rj_list.h"
#include "util/rj_queue.h"
#include "util/rj_mem_pool.h"
#include "util/logger.h"
#include "sys/sys_mem.h"
#include "sys/sys_pthread.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


///	@struct rj_conn_mem_t
///	@brief  单个网络连接的内存池
typedef struct rj_conn_mem_t
{
    char            *p_mem;
    uint32          mem_len;

    rj_mem_pool_h    mem_pool;
}rj_conn_mem_t;

///	@struct rj_net_ch_t
///	@brief  单个网络通道处理模块
typedef struct rj_net_ch_t
{
    rj_net_conn_t		*conn;			        ///< 绑定的使用环境对象(比如）
    rn_tcp_h            ws_tcp;                 ///< ws连接

    //完成一块的接收就把其放到rj_net_conn_t的recv_queue队列中
    _RJ_MEM_BLOCK_      *p_recv_buf;            ///< 接收缓存块
    int                 recving;                ///< 是否正在接收(0: 没有接收; 其他: 接收中)

    rj_net_w_h          wait_send;              ///< 待发送缓存块（一块未完成写入）
    rj_queue_h          send_queue;             ///< 待发送队列
    sys_mutex_t         *p_send_lock;           ///< 待发送锁[调用者, libuv线程] 保护send_queue, wait_send
    rj_net_r_h          sending_block;          ///< 正在发送的内存块（可能有多个rj_ndp_head_t包）
    int                 sending;                ///< 是否正在发送中(0: 不发送; 其他:发送中)
}rj_net_ch_t;

///	@struct rj_net_conn_t
///	@brief  单个网络连接处理模块
typedef struct rj_net_conn_t
{
    rj_net_m_conn_h	        multi_conn;				    ///< 多连接管理模块

    int                     conn_id;                    ///< 连接ID
    rn_state_e              conn_status;                ///< 连接状态（在线或者不在线）
    int                     b_will_stop;                ///< 标记是否处于关闭状态

    rj_list_h               conn_list;                  ///< 正在正常连接列表(rj_list_t)
    rj_list_h               close_list;                 ///< 待关闭的连接列表(rj_list_t)
    sys_mutex_t             *p_close_lock;              ///< 待关闭的连接锁[调用者, libuv线程]保护close_list

    rj_conn_mem_t           *p_send_mem;                ///< 发送内存池
    rj_conn_mem_t           *p_recv_mem;                ///< 接收内存池

    rj_queue_h              recv_queue;                 ///< 接收队列，供上层提取
    sys_mutex_t             *p_recv_lock;               ///< 接收锁[调用者, libuv线程]保护recv_queue
}rj_net_conn_t;


//////////////////////////////////////////////////////////////////////////
static void cb_close(rn_tcp_h tcp_h, void *obj);
rj_net_ch_t * find_net_ch(rj_list_h net_ch_list, uint32 ch_id);

//////////////////////////////////////////////////////////////////////////
//创建一个内存池
rj_conn_mem_t * conn_mem_create(uint32 s_buf_len, uint32 t_buf_len)
{
    rj_conn_mem_t *p_conn_mem = (rj_conn_mem_t*)sys_malloc(sizeof(rj_conn_mem_t));
    assert (NULL != p_conn_mem);

    p_conn_mem->mem_len = t_buf_len;
    p_conn_mem->p_mem   = (char*)sys_malloc(p_conn_mem->mem_len);
    assert (NULL != p_conn_mem->p_mem);

    p_conn_mem->mem_pool    = rj_mem_pool_create(p_conn_mem->p_mem, p_conn_mem->mem_len/s_buf_len, s_buf_len);
    assert (NULL != p_conn_mem->mem_pool);

    return p_conn_mem;
}

//销毁一个内存池
void conn_mem_destroy(rj_conn_mem_t *p_conn_mem)
{
    assert (NULL != p_conn_mem);
    if (NULL != p_conn_mem)
    {
        //销毁整个内存池
        rj_mem_pool_destroy(p_conn_mem->mem_pool);

        sys_free(p_conn_mem->p_mem);
        sys_free(p_conn_mem);
    }
}

//创建一个网络连接通道
rj_net_ch_t * net_ch_create(rj_net_conn_t *p_conn, rn_tcp_h ws_tcp)
{
    rj_net_ch_t *p_conn_ch = (rj_net_ch_t*)sys_malloc(sizeof(rj_net_ch_t));
    assert (NULL != p_conn_ch);

    p_conn_ch->conn             = p_conn;
    p_conn_ch->ws_tcp           = ws_tcp;

    p_conn_ch->p_recv_buf       = NULL;
    p_conn_ch->recving          = 0;
    
    p_conn_ch->wait_send        = NULL;
    p_conn_ch->sending_block    = NULL;
    p_conn_ch->sending          = 0;
    p_conn_ch->send_queue       = rj_queue_create();
    p_conn_ch->p_send_lock      = sys_mutex_create();

    assert (NULL != p_conn_ch->send_queue);

    return p_conn_ch;
}

//销毁一个网络连接通道
void net_ch_destroy(rj_net_ch_t * p_conn_ch)
{
    assert (NULL != p_conn_ch);

    if (NULL != p_conn_ch)
    {
        p_conn_ch->recving = 0;
        p_conn_ch->sending = 0;

        //释放接收缓存块
        if (NULL != p_conn_ch->p_recv_buf)
        {
            assert (NULL != p_conn_ch->conn->p_recv_mem);
            assert (NULL != p_conn_ch->conn->p_recv_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_recv_mem->mem_pool, p_conn_ch->p_recv_buf);
        }

        //释放正在准备的缓存块
        if (NULL != p_conn_ch->wait_send)
        {
            _RJ_MEM_BLOCK_ *p_block = rj_net_write_destroy(p_conn_ch->wait_send);

            assert (NULL != p_conn_ch->conn->p_send_mem);
            assert (NULL != p_conn_ch->conn->p_send_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_send_mem->mem_pool, p_block);
        }

        //释放所有的待发送缓存块
        void *p_buf = NULL;
        assert (NULL != p_conn_ch->send_queue);
        while (0 == rj_queue_pop(p_conn_ch->send_queue, &p_buf))
        {
            rj_net_r_h read_buf = (rj_net_r_h)(p_buf);
            assert(NULL != read_buf);
            _RJ_MEM_BLOCK_ *p_block = rj_net_read_destroy(read_buf);
            
            assert (NULL != p_conn_ch->conn->p_send_mem);
            assert (NULL != p_conn_ch->conn->p_send_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_send_mem->mem_pool, p_block);
        }

        rj_queue_destroy(p_conn_ch->send_queue);

        //释放正在发送的缓存块
        if (NULL != p_conn_ch->sending_block)
        {
            _RJ_MEM_BLOCK_ *p_block = rj_net_read_destroy(p_conn_ch->sending_block);

            assert (NULL != p_conn_ch->conn->p_send_mem);
            assert (NULL != p_conn_ch->conn->p_send_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_send_mem->mem_pool, p_block);
        }

        //释放锁
        sys_mutex_destroy(p_conn_ch->p_send_lock);

        sys_free(p_conn_ch);
    }
}

//在一个列表中找到一个网络连接通道
rj_net_ch_t * find_conn_ch(rj_list_h ch_list, int tag)
{
    assert (NULL != ch_list);
    if (NULL != ch_list)
    {
        rj_iterator iter = rj_list_begin(ch_list);
        while (iter != rj_list_end(ch_list)) 
        {
            rj_net_ch_t *p_ch = (rj_net_ch_t *)rj_iter_data(iter);;
            assert (NULL != p_ch);

            if (tag == rn_tcp_tag(p_ch->ws_tcp))
            {
                return p_ch;
            }

            iter = rj_iter_add(iter);
        }
    }

    return NULL;
}

//遍历一个列表，把所有的网络连接通道销毁
void destroy_all_ch(rj_list_h ch_list)
{
    assert (NULL != ch_list);
    if (NULL != ch_list)
    {
        while (0 < rj_list_size(ch_list))
        {
            rj_net_ch_t *p_ch = (rj_net_ch_t *)(rj_list_pop_front(ch_list));
            assert (NULL != p_ch);

            net_ch_destroy(p_ch);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
RJ_API rj_net_conn_h rj_conn_create(rj_net_m_conn_h m_conn, uint32 s_sbl, uint32 sbl, uint32 s_rbl, uint32 rbl)
{
    assert (NULL != m_conn);
    assert ((0 < s_sbl) && (s_sbl < sbl));
    assert ((0 < s_rbl) && (s_rbl < rbl));

    if (NULL != m_conn)
    {
        rj_net_conn_t *p_conn = (rj_net_conn_t*)sys_malloc(sizeof(rj_net_conn_t));
        assert (NULL != p_conn);

        p_conn->multi_conn  = m_conn;

        //初始化本连接的基本状态
        p_conn->conn_id     = -1;
        p_conn->conn_status = RN_CLOSE;
        p_conn->b_will_stop = 0;

        //初始化内存池
        p_conn->p_send_mem  = conn_mem_create(s_sbl, sbl);
        p_conn->p_recv_mem  = conn_mem_create(s_rbl, rbl);
        assert(NULL != p_conn->p_send_mem);
        assert(NULL != p_conn->p_recv_mem);

        //初始化接收队列
        p_conn->recv_queue = rj_queue_create();
        p_conn->p_recv_lock = sys_mutex_create();

        //初始化两个通道链表
        p_conn->conn_list   = rj_list_create();
        p_conn->close_list  = rj_list_create();
        assert(NULL != p_conn->conn_list);
        assert(NULL != p_conn->close_list);

        p_conn->p_close_lock = sys_mutex_create();
        assert(NULL != p_conn->p_close_lock);

        return (rj_net_conn_h)(p_conn);
    }
    
    return NULL;
}

RJ_API void rj_conn_destroy(rj_net_conn_h handle)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);

    assert (NULL != p_conn);
    if (NULL != p_conn)
    {
        //必须先销毁所有的通道
        assert(NULL != p_conn->conn_list);
        assert(NULL != p_conn->close_list);

        destroy_all_ch(p_conn->conn_list);
        destroy_all_ch(p_conn->close_list);

        rj_list_destroy(p_conn->conn_list);
        rj_list_destroy(p_conn->close_list);

        //释放所有的已经接收的缓存块
        void *p_buf = NULL;
        assert (NULL != p_conn->recv_queue);
        while (0 == rj_queue_pop(p_conn->recv_queue, &p_buf))
        {
            rj_net_r_h read_buf = (rj_net_r_h)(p_buf);
            assert(NULL != read_buf);
            _RJ_MEM_BLOCK_ *p_block = rj_net_read_destroy(read_buf);

            assert (NULL != p_conn->p_recv_mem);
            assert (NULL != p_conn->p_recv_mem->mem_pool);
            rj_mem_pool_free(p_conn->p_recv_mem->mem_pool, p_block);
        }

        rj_queue_destroy(p_conn->recv_queue);

        //释放内存池
        assert(NULL != p_conn->p_send_mem);
        assert(NULL != p_conn->p_recv_mem);
        conn_mem_destroy(p_conn->p_send_mem);
        conn_mem_destroy(p_conn->p_recv_mem);
        
        //释放锁
        sys_mutex_destroy(p_conn->p_recv_lock);
        sys_mutex_destroy(p_conn->p_close_lock);

        sys_free(p_conn);
    }
}

RJ_API int rj_conn_id(rj_net_conn_h handle)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);
    assert (NULL != p_conn);

    if (NULL != p_conn)
    {
        return p_conn->conn_id;
    }

    return RN_INVALID_TAG;
}

void rj_conn_set_id(rj_net_conn_h handle, int id)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);
    assert (NULL != p_conn);

    if (NULL != p_conn)
    {
        p_conn->conn_id = id;
    }
}

RJ_API void rj_conn_stop(rj_net_conn_h handle)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);

    // 先将所有的连接通道关闭
    while ( 0 < rj_list_size(p_conn->conn_list)) 
    {
        rj_net_ch_t *p_ch = (rj_net_ch_t *)rj_list_pop_front(p_conn->conn_list);
        assert(NULL != p_ch);

        if(NULL != p_ch)
        {
            // 加入关闭列表
            sys_mutex_lock(p_conn->p_close_lock);
            rj_list_push_back(p_conn->close_list, p_ch);
            sys_mutex_unlock(p_conn->p_close_lock);

            rn_tcp_close(p_ch->ws_tcp, cb_close, p_ch);
        }
    }

    p_conn->conn_status = RN_CLOSE;

    // 一直等到所有的连接关闭完成
    while(true)
    {
        // 等待libuv 退出完成
        sys_mutex_lock(p_conn->p_close_lock);
        int num = rj_list_size(p_conn->close_list);
        sys_mutex_unlock(p_conn->p_close_lock);

        if(num > 0)
        {
            sys_sleep(10);
        }
        else
        {
            break;
        }
    }

    assert (NULL != p_conn);
}

static void cb_close(rn_tcp_h tcp_h, void *obj)
{
    rj_net_ch_t *p_conn_ch = (rj_net_ch_t *)(obj);
    rj_net_conn_t *p_net_conn = p_conn_ch->conn;

    sys_mutex_lock(p_net_conn->p_close_lock);
    rj_list_remove(p_net_conn->close_list, p_conn_ch);
    sys_mutex_unlock(p_net_conn->p_close_lock);

    net_ch_destroy(p_conn_ch);
}

static void cb_alloc(rn_tcp_h tcp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf)
{
    rj_net_ch_t *p_conn_ch = (rj_net_ch_t *)(obj);
    rj_net_conn_t *p_net_conn = p_conn_ch->conn;
    assert(NULL != p_conn_ch);
    assert(NULL != p_net_conn);

    p_buf->base = NULL;
    p_buf->len = 0;

    assert(0 == p_conn_ch->recving);

    if ((NULL != p_conn_ch->p_recv_buf) &&
        (p_conn_ch->p_recv_buf->buf_len < (p_conn_ch->p_recv_buf->data_len + suggest_size)))
    {
        // 如果接接收缓存满了, 放入连接接收队列中
        assert(p_conn_ch->p_recv_buf->data_len > 0);
        rj_net_r_h net_r = rj_net_read_create(p_conn_ch->p_recv_buf, 0);
        p_conn_ch->p_recv_buf = NULL;

        sys_mutex_lock(p_net_conn->p_recv_lock);
        rj_queue_push(p_net_conn->recv_queue, net_r);
        sys_mutex_unlock(p_net_conn->p_recv_lock);
    }

    if(NULL == p_conn_ch->p_recv_buf)
    {
        // 如果没有接收缓存了, 申请一片
        p_conn_ch->p_recv_buf = rj_mem_pool_malloc(p_net_conn->p_recv_mem->mem_pool);
    }

    if((NULL != p_conn_ch->p_recv_buf) &&
       (p_conn_ch->p_recv_buf->buf_len >= (p_conn_ch->p_recv_buf->data_len + suggest_size)))
    {
        p_conn_ch->recving = 1;  // 标记正在读取

        p_buf->base = (char *)(p_conn_ch->p_recv_buf->p_buf + p_conn_ch->p_recv_buf->data_len);
        p_buf->len = suggest_size;
    }

    // 分配不到内存的条件
    // 1.没有申请到内存
    // 2.接收到的ws包大于内存池节点大小
}

static void cb_read(rn_tcp_h tcp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf)
{
    rj_net_ch_t *p_net_ch = (rj_net_ch_t *)(obj);
    rj_net_conn_t *p_net_conn = p_net_ch->conn;
    assert(NULL != p_net_ch);
    assert(NULL != p_net_conn);

    if(0 == nread)
    {
        if(NULL == p_buf)
        {
            // 网络读取空闲时
            // 将已经读取的数据放入待接收队列
            if( (NULL != p_net_ch->p_recv_buf) && 
                (0 < p_net_ch->p_recv_buf->data_len) )
            {
                rj_net_r_h net_r = rj_net_read_create(p_net_ch->p_recv_buf, 0);
                p_net_ch->p_recv_buf = NULL;

                sys_mutex_lock(p_net_conn->p_recv_lock);
                rj_queue_push(p_net_conn->recv_queue, net_r);
                sys_mutex_unlock(p_net_conn->p_recv_lock);
            }
        }
        else
        {
            // 没有读到数据
            // 不需要其他操作, 修改一下读取标记即可 
            p_net_ch->recving = 0;
        }
    }
    else if(0 < nread)
    {
        // 接收到数据
        assert(1 == p_net_ch->recving);
        p_net_ch->recving = 0;
        p_net_ch->p_recv_buf->data_len += nread;

    }
}

RJ_API int rj_conn_push_ch(rj_net_conn_h handle, rn_tcp_h ws_tcp)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);

    assert (NULL != p_conn);
    assert (NULL != ws_tcp);

    if ((NULL != p_conn) && (NULL != ws_tcp))
    {
        assert(NULL != p_conn->conn_list);
        
        rj_net_ch_t *p_net_ch = net_ch_create(p_conn, ws_tcp);
        assert(NULL != p_net_ch);

        // 开启读取
        if(RN_TCP_OK != rn_tcp_read_start(p_net_ch->ws_tcp, cb_alloc, cb_read, p_net_ch))
        {
            net_ch_destroy(p_net_ch);

            // 不应该出现的情况
            // 参数错误, tcp已经断开, 或libuv的run循环已经退出了
            assert(false);
            return 1;
        }

        p_conn->conn_status = RN_CONNECTED;
        rj_list_push_back(p_conn->conn_list, (void *)(p_net_ch));

        return 0;
    }

    return 1;
}

//只是把通道从正常列表移到待关闭列表
RJ_API void rj_conn_pop_ch(rj_net_conn_h handle, int ch_id)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);

    assert (NULL != p_conn);

    if (NULL != p_conn)
    {
        assert(NULL != p_conn->conn_list);
        assert(NULL != p_conn->close_list);

        rj_net_ch_t *p_net_ch = find_conn_ch(p_conn->conn_list, ch_id);
        if (NULL != p_net_ch)
        {
            // 移出正在连接列表
            rj_list_remove(p_conn->conn_list, p_net_ch);
            
            sys_mutex_lock(p_conn->p_close_lock);
            rj_list_push_back(p_conn->close_list, p_net_ch);
            sys_mutex_unlock(p_conn->p_close_lock);

            // 关闭tcp
            rn_tcp_close(p_net_ch->ws_tcp, cb_close, p_net_ch);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
rj_net_ch_t * find_net_ch(rj_list_h net_ch_list, int ch_id)
{
    rj_net_ch_t *p_net_ch = NULL;
    
    rj_iterator iter = rj_list_begin(net_ch_list);
    while (iter != rj_list_end(net_ch_list))
    {
        p_net_ch = (rj_net_ch_t *)rj_iter_data(iter);;
        if (ch_id == rn_tcp_tag(p_net_ch->ws_tcp))
        {
            return p_net_ch;
        }

        iter = rj_iter_add(iter);
    }

    return NULL;
}

static void cb_write(rn_tcp_h tcp_h, void *obj, rn_tcp_ret_e ret)
{
    rj_net_ch_t *p_net_ch = (rj_net_ch_t*)(obj);
    assert(NULL != p_net_ch);

    if(RN_TCP_FREE == ret)
    {
        // 写空闲时

        // 如果发送队列中没有数据, 而有残留等待发送的数据, 这时送入发送队列, 去发送
        sys_mutex_lock(p_net_ch->p_send_lock);
        if( 0 >= rj_queue_size(p_net_ch->send_queue) &&  (NULL != p_net_ch->wait_send))
        {
            //送入失败，则把当前的写缓存插入到待发送队列中去
            //并尝试重新申请一块内存
            _RJ_MEM_BLOCK_ * p_mem_block = rj_net_write_destroy(p_net_ch->wait_send);
            p_net_ch->wait_send = NULL;

            //插入到待发送队列
            assert (NULL != p_mem_block);
            rj_net_r_h net_read = rj_net_read_create(p_mem_block, 1);

            rj_queue_push(p_net_ch->send_queue, net_read);
        }
        sys_mutex_unlock(p_net_ch->p_send_lock);
    }

    if((RN_TCP_OK == ret) || (RN_TCP_FREE == ret))
    {
        // while 退出条件
        // 1. 找到一块可以发送的数据了
        // 2. 待发送的数据全部发送完毕
        while(true)
        {
            if(NULL == p_net_ch->sending_block)
            {
                void *p_node = NULL;

                sys_mutex_lock(p_net_ch->p_send_lock);
                if(0 == rj_queue_pop(p_net_ch->send_queue, &p_node))
                {
                    sys_mutex_unlock(p_net_ch->p_send_lock);
                    p_net_ch->sending_block = (rj_net_r_h)(p_node);
                }
                else
                {
                    sys_mutex_unlock(p_net_ch->p_send_lock);
                    // 没有数据可以发送了,退出while
                    break;
                }
            }

            if(NULL != p_net_ch->sending_block)
            {
                char *p_data = NULL;
                uint32 len = rj_net_read_pop(p_net_ch->sending_block, &p_data);

                if(len > 0)
                {
                    // 发送数据
                    uv_buf_t buf;
                    buf.base = (char *)(p_data);
                    buf.len = static_cast<uint_t>(len);

                    p_net_ch->sending = 1;

                    rn_tcp_write(p_net_ch->ws_tcp, &buf);

                    // 找到可以发送的数据了, 退出while
                    break;
                }
                else
                {
                    // 这个发送块中没有数据了, 可以释放了
                    _RJ_MEM_BLOCK_ *p_block = rj_net_read_destroy(p_net_ch->sending_block);

                    rj_net_conn_t *p_net_conn = (rj_net_conn_t*)(p_net_ch->conn);
                    assert(NULL != p_net_conn);
                    assert(NULL != p_net_conn->p_send_mem);

                    rj_mem_pool_free(p_net_conn->p_send_mem->mem_pool, p_block);

                    p_net_ch->sending_block = NULL;
                }
            }
        }
    }
}


static char get_ws_type_by_ndp(rj_ndp_pk_t *p_ndp)
{
    assert(NULL != p_ndp);

    switch(p_ndp->cat)
    {
    case RJNDP_CAT_SESSION:
    case RJNDP_CAT_EVENT:
        return WS_CMD;
        break;
    case RJNDP_CAT_TALK:
    case RJNDP_CAT_LIVE:
    case RJNDP_CAT_PLAYBACK:
    case RJNDP_CAT_BACKUP:
        return WS_BINARY;
        break;
    default:
        assert(false);
        return WS_BINARY;
        break;
    }

    assert(false);
    return WS_BINARY;
}


RJ_API int rj_conn_send(rj_net_conn_h handle, int ch_id, rj_ndp_pk_t *p_ndp, char *p_data)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);

    assert ((NULL != p_conn) && (NULL != p_ndp) && (NULL != p_data));
    assert ((NULL != p_ndp) && (0 < p_ndp->pk_len) && (p_ndp->pk_len <= RN_MAX_PACK_LEN));

    //先检查是否出错
    if ((NULL == p_conn) || (NULL == p_ndp) || (0 == p_ndp->pk_len) || (p_ndp->pk_len > RN_MAX_PACK_LEN) || (NULL == p_data))
    {
        return RN_TCP_PARAM_ERR;   //参数错误
    }

    //是否已经断开连接
    if( (0 >= rj_list_size(p_conn->conn_list)) || 
        (RN_CONNECTED != p_conn->conn_status) )
    {
        p_conn->conn_status = RN_CLOSE;
        return RN_TCP_DISCONNECT;
    }

    //首先检查指定的通道是否有效（存在且在连接状态）
    rj_net_ch_t *p_net_ch = find_net_ch(p_conn->conn_list, ch_id);
    if ((NULL == p_net_ch) || 
        (RN_CONNECTED != rn_tcp_state(p_net_ch->ws_tcp)))
    {
        // 没有找到通道
        return RN_TCP_CH_DISCONNECT;
    }

    sys_mutex_lock(p_net_ch->p_send_lock);

    //没有发送缓存,尝试申请一个
    if (NULL == p_net_ch->wait_send)
    {
        _RJ_MEM_BLOCK_ *p_mem_block = rj_mem_pool_malloc(p_conn->p_send_mem->mem_pool);
        if (NULL == p_mem_block)
        {
            sys_mutex_unlock(p_net_ch->p_send_lock);
            LOG_WARN("cannot get memcpy from pool\n");
            return RN_TCP_OUT_OF_MEMORY;//暂时无内存
        }

        //初始化发送缓存
        p_net_ch->wait_send   = rj_net_write_create(p_mem_block);
    }

    assert (NULL != p_net_ch->wait_send);
    if (0 != rj_net_write_push(p_net_ch->wait_send, p_ndp, p_data, get_ws_type_by_ndp(p_ndp)))
    {
        //送入失败，则把当前的写缓存插入到待发送队列中去
        //并尝试重新申请一块内存
        _RJ_MEM_BLOCK_ * p_mem_block = rj_net_write_destroy(p_net_ch->wait_send);

        //插入到待发送队列
        assert (NULL != p_mem_block);
        rj_net_r_h net_read = rj_net_read_create(p_mem_block, 1);
               
        rj_queue_push(p_net_ch->send_queue, net_read);

        //重新从内存池分配内存块
        p_mem_block = rj_mem_pool_malloc(p_conn->p_send_mem->mem_pool);
        if (NULL != p_mem_block)
        {
            p_net_ch->wait_send = rj_net_write_create(p_mem_block);

            if (0 != rj_net_write_push(p_net_ch->wait_send, p_ndp, p_data, get_ws_type_by_ndp(p_ndp)))
            {
                sys_mutex_unlock(p_net_ch->p_send_lock);
                return RN_TCP_PARAM_ERR;   //一个包大于最大缓存块，不能发送，要求从设计上限制出现这种情况
            }
            else
            {
                // 通知libuv线程处理发送
                rn_tcp_try_write(p_net_ch->ws_tcp, cb_write, p_net_ch);

                sys_mutex_unlock(p_net_ch->p_send_lock);
                return RN_TCP_OK;
            }
        }
        else
        {
            //分配不成功
            p_net_ch->wait_send   = NULL;

            sys_mutex_unlock(p_net_ch->p_send_lock);
            return RN_TCP_OUT_OF_MEMORY;
        }
    }
    else
    {
        // 通知libuv线程处理发送
        rn_tcp_try_write(p_net_ch->ws_tcp, cb_write, p_net_ch);

        sys_mutex_unlock(p_net_ch->p_send_lock);
        return RN_TCP_OK;
    }

    sys_mutex_unlock(p_net_ch->p_send_lock);

    return RN_TCP_OK;
}

RJ_API int rj_conn_recv(rj_net_conn_h handle, rj_net_r_h *p_recv_data)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);
    assert (NULL != p_conn);
    assert (NULL != p_recv_data);

    if ((NULL != p_conn) && (NULL != p_recv_data))
    {
        // 检查正在连接的通道
        // 如果存在已经断开的连接, 将连接移动关闭列表, 并关闭tcp
        rj_iterator iter = rj_list_begin(p_conn->conn_list);
        rj_iterator end_iter = rj_list_end(p_conn->conn_list);

        while (iter != end_iter)
        {
            rj_iterator tmp = iter;
            iter = rj_iter_add(iter);

            rj_net_ch_t *p_tmp_ch = (rj_net_ch_t *)rj_iter_data(tmp);

            int state = rn_tcp_state(p_tmp_ch->ws_tcp);

            if(RN_CONNECTED != state)
            {
                rj_list_remove(p_conn->conn_list, p_tmp_ch);
                
                sys_mutex_lock(p_conn->p_close_lock);
                rj_list_push_back(p_conn->close_list, p_tmp_ch);
                sys_mutex_unlock(p_conn->p_close_lock);

                // 关闭tcp
                rn_tcp_close(p_tmp_ch->ws_tcp, cb_close, p_tmp_ch);
            }
        }

        //   1.状态为断开
        //   2.正常的连接列表为空 p_conn->conn_list
        //   则清理缓存的数据
        if( (RN_CONNECTED != p_conn->conn_status) ||
            (0 >= rj_list_size(p_conn->conn_list)) )
        {
            p_conn->conn_status = RN_CLOSE;

            sys_mutex_lock(p_conn->p_recv_lock);
            
            // 清空残留的数据
            while(true)
            {
                void *p_tmp = NULL;
                if (0 == rj_queue_pop(p_conn->recv_queue, &p_tmp))
                {
                    assert(NULL != p_tmp);
                    rj_net_r_h net_r_tmp = (rj_net_r_h)p_tmp;

                    _RJ_MEM_BLOCK_ *p_block_tmp = rj_net_read_destroy(net_r_tmp);

                    // 归还给内存池
                    assert (NULL != p_conn->p_recv_mem);
                    assert (NULL != p_conn->p_recv_mem->mem_pool);
                    rj_mem_pool_free(p_conn->p_recv_mem->mem_pool, p_block_tmp);
                }
                else
                {
                    break;
                }
            }

            sys_mutex_unlock(p_conn->p_recv_lock);

            return RN_TCP_DISCONNECT;
        }

        //遍历待读取队列, 取出数据
        assert (NULL != p_conn->recv_queue);
        void *p_data = NULL;

        sys_mutex_lock(p_conn->p_recv_lock);
        if (0 == rj_queue_pop(p_conn->recv_queue, &p_data))
        {
            sys_mutex_unlock(p_conn->p_recv_lock);
            *p_recv_data    = (rj_net_r_h)(p_data);
            return RN_TCP_OK;
        }

        sys_mutex_unlock(p_conn->p_recv_lock);
    }

    *p_recv_data = NULL;
    return RN_TCP_OTHER;
}

RJ_API void rj_conn_free_mem(rj_net_conn_h handle, rj_net_r_h recv_data)
{
    rj_net_conn_t *p_conn = (rj_net_conn_t *)(handle);
    assert (NULL != p_conn);
    assert (NULL != recv_data);

    if ((NULL != p_conn) && (NULL != recv_data))
    {
        //先销毁读缓存的句柄
        _RJ_MEM_BLOCK_ *p_mem_block = rj_net_read_destroy(recv_data);

        //归还给内存池
        assert (NULL != p_conn->p_recv_mem);
        assert (NULL != p_conn->p_recv_mem->mem_pool);
        rj_mem_pool_free(p_conn->p_recv_mem->mem_pool, p_mem_block);
    }
}

//end
