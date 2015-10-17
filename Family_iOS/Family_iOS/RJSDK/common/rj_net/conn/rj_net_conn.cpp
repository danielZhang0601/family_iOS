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
///	@brief  �����������ӵ��ڴ��
typedef struct rj_conn_mem_t
{
    char            *p_mem;
    uint32          mem_len;

    rj_mem_pool_h    mem_pool;
}rj_conn_mem_t;

///	@struct rj_net_ch_t
///	@brief  ��������ͨ������ģ��
typedef struct rj_net_ch_t
{
    rj_net_conn_t		*conn;			        ///< �󶨵�ʹ�û�������(���磩
    rn_tcp_h            ws_tcp;                 ///< ws����

    //���һ��Ľ��վͰ���ŵ�rj_net_conn_t��recv_queue������
    _RJ_MEM_BLOCK_      *p_recv_buf;            ///< ���ջ����
    int                 recving;                ///< �Ƿ����ڽ���(0: û�н���; ����: ������)

    rj_net_w_h          wait_send;              ///< �����ͻ���飨һ��δ���д�룩
    rj_queue_h          send_queue;             ///< �����Ͷ���
    sys_mutex_t         *p_send_lock;           ///< ��������[������, libuv�߳�] ����send_queue, wait_send
    rj_net_r_h          sending_block;          ///< ���ڷ��͵��ڴ�飨�����ж��rj_ndp_head_t����
    int                 sending;                ///< �Ƿ����ڷ�����(0: ������; ����:������)
}rj_net_ch_t;

///	@struct rj_net_conn_t
///	@brief  �����������Ӵ���ģ��
typedef struct rj_net_conn_t
{
    rj_net_m_conn_h	        multi_conn;				    ///< �����ӹ���ģ��

    int                     conn_id;                    ///< ����ID
    rn_state_e              conn_status;                ///< ����״̬�����߻��߲����ߣ�
    int                     b_will_stop;                ///< ����Ƿ��ڹر�״̬

    rj_list_h               conn_list;                  ///< �������������б�(rj_list_t)
    rj_list_h               close_list;                 ///< ���رյ������б�(rj_list_t)
    sys_mutex_t             *p_close_lock;              ///< ���رյ�������[������, libuv�߳�]����close_list

    rj_conn_mem_t           *p_send_mem;                ///< �����ڴ��
    rj_conn_mem_t           *p_recv_mem;                ///< �����ڴ��

    rj_queue_h              recv_queue;                 ///< ���ն��У����ϲ���ȡ
    sys_mutex_t             *p_recv_lock;               ///< ������[������, libuv�߳�]����recv_queue
}rj_net_conn_t;


//////////////////////////////////////////////////////////////////////////
static void cb_close(rn_tcp_h tcp_h, void *obj);
rj_net_ch_t * find_net_ch(rj_list_h net_ch_list, uint32 ch_id);

//////////////////////////////////////////////////////////////////////////
//����һ���ڴ��
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

//����һ���ڴ��
void conn_mem_destroy(rj_conn_mem_t *p_conn_mem)
{
    assert (NULL != p_conn_mem);
    if (NULL != p_conn_mem)
    {
        //���������ڴ��
        rj_mem_pool_destroy(p_conn_mem->mem_pool);

        sys_free(p_conn_mem->p_mem);
        sys_free(p_conn_mem);
    }
}

//����һ����������ͨ��
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

//����һ����������ͨ��
void net_ch_destroy(rj_net_ch_t * p_conn_ch)
{
    assert (NULL != p_conn_ch);

    if (NULL != p_conn_ch)
    {
        p_conn_ch->recving = 0;
        p_conn_ch->sending = 0;

        //�ͷŽ��ջ����
        if (NULL != p_conn_ch->p_recv_buf)
        {
            assert (NULL != p_conn_ch->conn->p_recv_mem);
            assert (NULL != p_conn_ch->conn->p_recv_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_recv_mem->mem_pool, p_conn_ch->p_recv_buf);
        }

        //�ͷ�����׼���Ļ����
        if (NULL != p_conn_ch->wait_send)
        {
            _RJ_MEM_BLOCK_ *p_block = rj_net_write_destroy(p_conn_ch->wait_send);

            assert (NULL != p_conn_ch->conn->p_send_mem);
            assert (NULL != p_conn_ch->conn->p_send_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_send_mem->mem_pool, p_block);
        }

        //�ͷ����еĴ����ͻ����
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

        //�ͷ����ڷ��͵Ļ����
        if (NULL != p_conn_ch->sending_block)
        {
            _RJ_MEM_BLOCK_ *p_block = rj_net_read_destroy(p_conn_ch->sending_block);

            assert (NULL != p_conn_ch->conn->p_send_mem);
            assert (NULL != p_conn_ch->conn->p_send_mem->mem_pool);
            rj_mem_pool_free(p_conn_ch->conn->p_send_mem->mem_pool, p_block);
        }

        //�ͷ���
        sys_mutex_destroy(p_conn_ch->p_send_lock);

        sys_free(p_conn_ch);
    }
}

//��һ���б����ҵ�һ����������ͨ��
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

//����һ���б������е���������ͨ������
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

        //��ʼ�������ӵĻ���״̬
        p_conn->conn_id     = -1;
        p_conn->conn_status = RN_CLOSE;
        p_conn->b_will_stop = 0;

        //��ʼ���ڴ��
        p_conn->p_send_mem  = conn_mem_create(s_sbl, sbl);
        p_conn->p_recv_mem  = conn_mem_create(s_rbl, rbl);
        assert(NULL != p_conn->p_send_mem);
        assert(NULL != p_conn->p_recv_mem);

        //��ʼ�����ն���
        p_conn->recv_queue = rj_queue_create();
        p_conn->p_recv_lock = sys_mutex_create();

        //��ʼ������ͨ������
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
        //�������������е�ͨ��
        assert(NULL != p_conn->conn_list);
        assert(NULL != p_conn->close_list);

        destroy_all_ch(p_conn->conn_list);
        destroy_all_ch(p_conn->close_list);

        rj_list_destroy(p_conn->conn_list);
        rj_list_destroy(p_conn->close_list);

        //�ͷ����е��Ѿ����յĻ����
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

        //�ͷ��ڴ��
        assert(NULL != p_conn->p_send_mem);
        assert(NULL != p_conn->p_recv_mem);
        conn_mem_destroy(p_conn->p_send_mem);
        conn_mem_destroy(p_conn->p_recv_mem);
        
        //�ͷ���
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

    // �Ƚ����е�����ͨ���ر�
    while ( 0 < rj_list_size(p_conn->conn_list)) 
    {
        rj_net_ch_t *p_ch = (rj_net_ch_t *)rj_list_pop_front(p_conn->conn_list);
        assert(NULL != p_ch);

        if(NULL != p_ch)
        {
            // ����ر��б�
            sys_mutex_lock(p_conn->p_close_lock);
            rj_list_push_back(p_conn->close_list, p_ch);
            sys_mutex_unlock(p_conn->p_close_lock);

            rn_tcp_close(p_ch->ws_tcp, cb_close, p_ch);
        }
    }

    p_conn->conn_status = RN_CLOSE;

    // һֱ�ȵ����е����ӹر����
    while(true)
    {
        // �ȴ�libuv �˳����
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
        // ����ӽ��ջ�������, �������ӽ��ն�����
        assert(p_conn_ch->p_recv_buf->data_len > 0);
        rj_net_r_h net_r = rj_net_read_create(p_conn_ch->p_recv_buf, 0);
        p_conn_ch->p_recv_buf = NULL;

        sys_mutex_lock(p_net_conn->p_recv_lock);
        rj_queue_push(p_net_conn->recv_queue, net_r);
        sys_mutex_unlock(p_net_conn->p_recv_lock);
    }

    if(NULL == p_conn_ch->p_recv_buf)
    {
        // ���û�н��ջ�����, ����һƬ
        p_conn_ch->p_recv_buf = rj_mem_pool_malloc(p_net_conn->p_recv_mem->mem_pool);
    }

    if((NULL != p_conn_ch->p_recv_buf) &&
       (p_conn_ch->p_recv_buf->buf_len >= (p_conn_ch->p_recv_buf->data_len + suggest_size)))
    {
        p_conn_ch->recving = 1;  // ������ڶ�ȡ

        p_buf->base = (char *)(p_conn_ch->p_recv_buf->p_buf + p_conn_ch->p_recv_buf->data_len);
        p_buf->len = suggest_size;
    }

    // ���䲻���ڴ������
    // 1.û�����뵽�ڴ�
    // 2.���յ���ws�������ڴ�ؽڵ��С
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
            // �����ȡ����ʱ
            // ���Ѿ���ȡ�����ݷ�������ն���
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
            // û�ж�������
            // ����Ҫ��������, �޸�һ�¶�ȡ��Ǽ��� 
            p_net_ch->recving = 0;
        }
    }
    else if(0 < nread)
    {
        // ���յ�����
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

        // ������ȡ
        if(RN_TCP_OK != rn_tcp_read_start(p_net_ch->ws_tcp, cb_alloc, cb_read, p_net_ch))
        {
            net_ch_destroy(p_net_ch);

            // ��Ӧ�ó��ֵ����
            // ��������, tcp�Ѿ��Ͽ�, ��libuv��runѭ���Ѿ��˳���
            assert(false);
            return 1;
        }

        p_conn->conn_status = RN_CONNECTED;
        rj_list_push_back(p_conn->conn_list, (void *)(p_net_ch));

        return 0;
    }

    return 1;
}

//ֻ�ǰ�ͨ���������б��Ƶ����ر��б�
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
            // �Ƴ����������б�
            rj_list_remove(p_conn->conn_list, p_net_ch);
            
            sys_mutex_lock(p_conn->p_close_lock);
            rj_list_push_back(p_conn->close_list, p_net_ch);
            sys_mutex_unlock(p_conn->p_close_lock);

            // �ر�tcp
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
        // д����ʱ

        // ������Ͷ�����û������, ���в����ȴ����͵�����, ��ʱ���뷢�Ͷ���, ȥ����
        sys_mutex_lock(p_net_ch->p_send_lock);
        if( 0 >= rj_queue_size(p_net_ch->send_queue) &&  (NULL != p_net_ch->wait_send))
        {
            //����ʧ�ܣ���ѵ�ǰ��д������뵽�����Ͷ�����ȥ
            //��������������һ���ڴ�
            _RJ_MEM_BLOCK_ * p_mem_block = rj_net_write_destroy(p_net_ch->wait_send);
            p_net_ch->wait_send = NULL;

            //���뵽�����Ͷ���
            assert (NULL != p_mem_block);
            rj_net_r_h net_read = rj_net_read_create(p_mem_block, 1);

            rj_queue_push(p_net_ch->send_queue, net_read);
        }
        sys_mutex_unlock(p_net_ch->p_send_lock);
    }

    if((RN_TCP_OK == ret) || (RN_TCP_FREE == ret))
    {
        // while �˳�����
        // 1. �ҵ�һ����Է��͵�������
        // 2. �����͵�����ȫ���������
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
                    // û�����ݿ��Է�����,�˳�while
                    break;
                }
            }

            if(NULL != p_net_ch->sending_block)
            {
                char *p_data = NULL;
                uint32 len = rj_net_read_pop(p_net_ch->sending_block, &p_data);

                if(len > 0)
                {
                    // ��������
                    uv_buf_t buf;
                    buf.base = (char *)(p_data);
                    buf.len = static_cast<uint_t>(len);

                    p_net_ch->sending = 1;

                    rn_tcp_write(p_net_ch->ws_tcp, &buf);

                    // �ҵ����Է��͵�������, �˳�while
                    break;
                }
                else
                {
                    // ������Ϳ���û��������, �����ͷ���
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

    //�ȼ���Ƿ����
    if ((NULL == p_conn) || (NULL == p_ndp) || (0 == p_ndp->pk_len) || (p_ndp->pk_len > RN_MAX_PACK_LEN) || (NULL == p_data))
    {
        return RN_TCP_PARAM_ERR;   //��������
    }

    //�Ƿ��Ѿ��Ͽ�����
    if( (0 >= rj_list_size(p_conn->conn_list)) || 
        (RN_CONNECTED != p_conn->conn_status) )
    {
        p_conn->conn_status = RN_CLOSE;
        return RN_TCP_DISCONNECT;
    }

    //���ȼ��ָ����ͨ���Ƿ���Ч��������������״̬��
    rj_net_ch_t *p_net_ch = find_net_ch(p_conn->conn_list, ch_id);
    if ((NULL == p_net_ch) || 
        (RN_CONNECTED != rn_tcp_state(p_net_ch->ws_tcp)))
    {
        // û���ҵ�ͨ��
        return RN_TCP_CH_DISCONNECT;
    }

    sys_mutex_lock(p_net_ch->p_send_lock);

    //û�з��ͻ���,��������һ��
    if (NULL == p_net_ch->wait_send)
    {
        _RJ_MEM_BLOCK_ *p_mem_block = rj_mem_pool_malloc(p_conn->p_send_mem->mem_pool);
        if (NULL == p_mem_block)
        {
            sys_mutex_unlock(p_net_ch->p_send_lock);
            LOG_WARN("cannot get memcpy from pool\n");
            return RN_TCP_OUT_OF_MEMORY;//��ʱ���ڴ�
        }

        //��ʼ�����ͻ���
        p_net_ch->wait_send   = rj_net_write_create(p_mem_block);
    }

    assert (NULL != p_net_ch->wait_send);
    if (0 != rj_net_write_push(p_net_ch->wait_send, p_ndp, p_data, get_ws_type_by_ndp(p_ndp)))
    {
        //����ʧ�ܣ���ѵ�ǰ��д������뵽�����Ͷ�����ȥ
        //��������������һ���ڴ�
        _RJ_MEM_BLOCK_ * p_mem_block = rj_net_write_destroy(p_net_ch->wait_send);

        //���뵽�����Ͷ���
        assert (NULL != p_mem_block);
        rj_net_r_h net_read = rj_net_read_create(p_mem_block, 1);
               
        rj_queue_push(p_net_ch->send_queue, net_read);

        //���´��ڴ�ط����ڴ��
        p_mem_block = rj_mem_pool_malloc(p_conn->p_send_mem->mem_pool);
        if (NULL != p_mem_block)
        {
            p_net_ch->wait_send = rj_net_write_create(p_mem_block);

            if (0 != rj_net_write_push(p_net_ch->wait_send, p_ndp, p_data, get_ws_type_by_ndp(p_ndp)))
            {
                sys_mutex_unlock(p_net_ch->p_send_lock);
                return RN_TCP_PARAM_ERR;   //һ����������󻺴�飬���ܷ��ͣ�Ҫ�����������Ƴ����������
            }
            else
            {
                // ֪ͨlibuv�̴߳�����
                rn_tcp_try_write(p_net_ch->ws_tcp, cb_write, p_net_ch);

                sys_mutex_unlock(p_net_ch->p_send_lock);
                return RN_TCP_OK;
            }
        }
        else
        {
            //���䲻�ɹ�
            p_net_ch->wait_send   = NULL;

            sys_mutex_unlock(p_net_ch->p_send_lock);
            return RN_TCP_OUT_OF_MEMORY;
        }
    }
    else
    {
        // ֪ͨlibuv�̴߳�����
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
        // ����������ӵ�ͨ��
        // ��������Ѿ��Ͽ�������, �������ƶ��ر��б�, ���ر�tcp
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

                // �ر�tcp
                rn_tcp_close(p_tmp_ch->ws_tcp, cb_close, p_tmp_ch);
            }
        }

        //   1.״̬Ϊ�Ͽ�
        //   2.�����������б�Ϊ�� p_conn->conn_list
        //   �������������
        if( (RN_CONNECTED != p_conn->conn_status) ||
            (0 >= rj_list_size(p_conn->conn_list)) )
        {
            p_conn->conn_status = RN_CLOSE;

            sys_mutex_lock(p_conn->p_recv_lock);
            
            // ��ղ���������
            while(true)
            {
                void *p_tmp = NULL;
                if (0 == rj_queue_pop(p_conn->recv_queue, &p_tmp))
                {
                    assert(NULL != p_tmp);
                    rj_net_r_h net_r_tmp = (rj_net_r_h)p_tmp;

                    _RJ_MEM_BLOCK_ *p_block_tmp = rj_net_read_destroy(net_r_tmp);

                    // �黹���ڴ��
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

        //��������ȡ����, ȡ������
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
        //�����ٶ�����ľ��
        _RJ_MEM_BLOCK_ *p_mem_block = rj_net_read_destroy(recv_data);

        //�黹���ڴ��
        assert (NULL != p_conn->p_recv_mem);
        assert (NULL != p_conn->p_recv_mem->mem_pool);
        rj_mem_pool_free(p_conn->p_recv_mem->mem_pool, p_mem_block);
    }
}

//end
