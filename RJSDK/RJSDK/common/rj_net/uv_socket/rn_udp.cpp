#include "rn_udp.h"
#include <assert.h>
#include <string.h>
#include "util/logger.h"

//////////////////////////////////////////////////////////////////////////
typedef struct rn_udp_t
{
    uint16				state;				///< 0：表示为开启连接
    uint16				port;				///< 

    uv_loop_t           *p_loop;            ///< LibUV的对象指针
    uv_udp_t		    uv_conn;			///< 连接对象

	struct      ///< 用于写数据的消息和回调
    {
        uint16              rsv;
        uint16              sending;

        udp_cb_send	        cb;
        void			    *obj;
        uv_async_t          *p_async;
        uv_udp_send_t       send;
    }send;

    struct      ///< 用于读数据的消息和回调
    {
        udp_cb_alloc	    cb_alloc;
        udp_cb_recv	        cb_recv;
        void			    *obj;
        uv_async_t          *p_async;
    }recv;
}rn_udp_t;

//////////////////////////////////////////////////////////////////////////
static void cb_send(uv_udp_send_t* req, int status)
{
    assert (NULL != req);
    rn_udp_t *p_udp = (rn_udp_t *)req->data;
    assert (NULL != p_udp);
    assert ((NULL != p_udp->send.cb) && (NULL != p_udp->send.obj));
    assert (1 == p_udp->send.sending);
    p_udp->send.sending   = 0;
}

static void cb_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    assert (NULL != handle);
    rn_udp_t *p_udp = (rn_udp_t *)handle->data;
    assert (NULL != p_udp);
    assert ((NULL != p_udp->recv.cb_alloc) && (NULL != p_udp->recv.obj));

    p_udp->recv.cb_alloc((rn_udp_h)p_udp, p_udp->recv.obj, suggested_size, buf);
}

static void cb_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags)
{
    assert (NULL != handle);
    rn_udp_t *p_udp = (rn_udp_t *)handle->data;
    assert (NULL != p_udp);
    assert ((NULL != p_udp->recv.cb_recv) && (NULL != p_udp->recv.obj));

    if (nread < 0)
    {
        p_udp->recv.cb_recv((rn_udp_h)p_udp, p_udp->recv.obj, nread, NULL, NULL);
        assert (0);
    }
    else if (0 < nread)
    {
        p_udp->recv.cb_recv((rn_udp_h)p_udp, p_udp->recv.obj, nread, buf, addr);
    }
}

//////////////////////////////////////////////////////////////////////////

static void on_write(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_udp_t *p_udp = (rn_udp_t *)handle->data;
    assert (NULL != p_udp);

    if ((NULL != p_udp->send.cb) && (NULL != p_udp->send.obj))
    {
        assert ((NULL != p_udp->send.cb) && (NULL != p_udp->send.obj));

        if (0 == p_udp->send.sending)
            p_udp->send.cb((rn_udp_h)p_udp, p_udp->send.obj, RN_UDP_OK);
    }
}

static void on_recv(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_udp_t *p_udp = (rn_udp_t *)handle->data;
    assert (NULL != p_udp);
    assert ((NULL != p_udp->recv.cb_alloc) && (NULL != p_udp->recv.cb_recv) && (NULL != p_udp->recv.obj));

    //开启读
    if (0 == p_udp->state)
    {
        if (0 != uv_udp_recv_start(&p_udp->uv_conn, cb_alloc, cb_recv))
            assert (0);
        else
            p_udp->state    = 1;
    }
    else
    {
        //停止读
        if (0 != uv_udp_recv_stop(&p_udp->uv_conn))
            assert (0);
        else
            p_udp->state    = 0;
    }
}
//////////////////////////////////////////////////////////////////////////
rn_udp_h rn_udp_create(uv_loop_t  *p_loop, uint16 port)
{
    assert (NULL != p_loop);

    if (NULL != p_loop)
    {
        rn_udp_t *p_udp = new rn_udp_t;
        memset(p_udp, 0, sizeof(rn_udp_t));
        
        p_udp->port     = port;
        p_udp->p_loop   = p_loop;
        p_udp->uv_conn.data = p_udp;
        uv_udp_init(p_loop, &p_udp->uv_conn);

        //初始化信号量
        p_udp->send.send.data     = p_udp;
        p_udp->send.p_async        = new uv_async_t;
        p_udp->send.p_async->data  = p_udp;
        uv_async_init(p_udp->p_loop, p_udp->send.p_async, on_write);

        p_udp->recv.p_async         = new uv_async_t;
        p_udp->recv.p_async->data   = p_udp;
        uv_async_init(p_udp->p_loop, p_udp->recv.p_async, on_recv);

        //这个是打开广播？
		struct sockaddr_in	broadcastSendAddr = {0};
		broadcastSendAddr.sin_family			= AF_INET;  
		broadcastSendAddr.sin_addr.s_addr	= htonl(INADDR_ANY);  
		broadcastSendAddr.sin_port		    = htons(port);
		if(0 != uv_udp_bind(&p_udp->uv_conn,(sockaddr *)&broadcastSendAddr,0))
		{
			assert(false);
		}

        if(0 != uv_udp_set_broadcast(&p_udp->uv_conn, 1))
		{
			assert(false);
		}

        return (rn_udp_h)p_udp;
    }

    return NULL;
}

void rn_udp_destroy(rn_udp_h udp_h)
{
    assert (NULL != udp_h);
    rn_udp_t *p_udp = (rn_udp_t *)udp_h;
    assert (NULL != p_udp);

    uv_close((uv_handle_t *)&p_udp->uv_conn, NULL);

    if (NULL != p_udp->send.p_async)
    {
        delete p_udp->send.p_async;
    }

    if (NULL != p_udp->recv.p_async)
    {
        delete p_udp->recv.p_async;
    }

    delete p_udp;
}

int rn_udp_try_write(rn_udp_h udp_h, udp_cb_send cb, void *cb_obj)
{
    assert (NULL != udp_h);
    assert (NULL != cb);
    assert (NULL != cb_obj);

    rn_udp_t *p_udp = (rn_udp_t *)udp_h;

    if ((NULL != p_udp) && (NULL != cb) && (NULL != cb_obj))
    {
        p_udp->send.cb             = cb;
        p_udp->send.obj            = cb_obj;

        assert (NULL != p_udp->send.p_async);
        assert (p_udp == p_udp->send.p_async->data);
        assert (p_udp == p_udp->send.send.data);

        if (NULL != p_udp->send.p_async)
            uv_async_send(p_udp->send.p_async);

        return RN_UDP_OK;
    }

    return RN_UDP_PARAM_ERR;
}

int rn_udp_send(rn_udp_h udp_h, struct sockaddr * addr, uv_buf_t * p_data)
{
    assert (NULL != udp_h);
    assert (NULL != p_data);

    rn_udp_t *p_udp = (rn_udp_t *)udp_h;

    if ((NULL != p_udp) && (NULL != p_data))
    {
        p_udp->send.sending   = 1;

        if (0 != uv_udp_send(&p_udp->send.send, &p_udp->uv_conn, p_data, 1, addr, cb_send))
        {
            return RN_UDP_TIMEOUT;
        }

        return RN_UDP_OK;
    }

    return RN_UDP_PARAM_ERR;
}

int rn_udp_read_start(rn_udp_h udp_h, udp_cb_alloc cb_alloc, udp_cb_recv cb_recv, void *cb_obj)
{
    assert (NULL != udp_h);
    assert (NULL != cb_alloc);
    assert (NULL != cb_recv);
    assert (NULL != cb_obj);

    rn_udp_t *p_udp = (rn_udp_t *)udp_h;

    if ((NULL != p_udp) && (NULL != cb_alloc) && (NULL != cb_recv) && (NULL != cb_obj))
    {
        p_udp->recv.cb_alloc        = cb_alloc;
        p_udp->recv.cb_recv         = cb_recv;
        p_udp->recv.obj             = cb_obj;

        assert (NULL != p_udp->recv.p_async);
        assert (p_udp->recv.p_async->data == p_udp);

        if (NULL != p_udp->recv.p_async)
            uv_async_send(p_udp->recv.p_async);

        return RN_UDP_OK;
    }

    return RN_UDP_PARAM_ERR;
}

void rn_udp_read_stop(rn_udp_h udp_h)
{
    assert (NULL != udp_h);

    rn_udp_t *p_udp = (rn_udp_t *)udp_h;

    if (NULL != p_udp)
    {
        p_udp->recv.cb_alloc        = NULL;
        p_udp->recv.cb_recv         = NULL;
        p_udp->recv.obj             = NULL;

        assert (NULL != p_udp->recv.p_async);
        assert (p_udp->recv.p_async->data == p_udp);

        if (NULL != p_udp->recv.p_async)
            uv_async_send(p_udp->recv.p_async);
    }
}


//end
