#include "rn_tcp.h"
#include "rn_ws_parser.h"
#include "util/rj_queue.h"
#include "pub/pub_define.h"
#include <assert.h>
#include <string.h>

const   uint64_t RN_TIMER_TIMEOUT   = 2000;
const   uint64_t RN_TIMER_REPEAT    = 1000;

const   uint16  RN_WS_HS_BUF_LEN    = 256;
const   uint16  RN_WS_CMD_BUF_LEN   = 64;

const   int     RN_TIMEOUT          = 10000;//10秒

int     g_rn_tcp_tag                = 100;

typedef enum rn_tcp_mode_e
{
    RN_TCP_SERVER       = 0,
    RN_TCP_CLIENT
}rn_tcp_mode_e;

typedef struct rn_tcp_ws_t
{
    uint16              b_hs_ed;        ///< 是否握手完成
    uint16              rsv;

    tcp_ws_cb_hs        cb;             ///< 握手回调
    void                *obj;           ///< 所属对象的指针

    size_t              cmd_len;        ///< 已经收取的控制缓存中的数据长度
    uv_buf_t            cmd;            ///< 专门用于发送控制命令的
    uv_write_t          cmd_write;      ///< 专门用于发送控制命令
    uv_buf_t            write_buf;      ///< 用于发送数据

    char                exp_key[32];    ///< 握手协商的秘钥
    char                protocol[32];   ///< 子协议

    struct 
    {
        uint16              rsv;
        uint16              head;           ///< 是否接到了一个WS头，0表示没有头
        
        size_t              last_len;       ///< 最近一个包已经收取的长度
        size_t              ing_len;        ///< 数据结束位置
        size_t              ed_len;         ///< 接收完成且解密的数据长度
        uv_buf_t            ing;            ///< 正在接收的缓存区
        uv_buf_t            ed;             ///< 已经解密后的数据
    }recv;

    struct  
    {
        char           fin;
        char           rsv1;
        char           opcode;
        char           has_key;
        char           key[4];
        uint64         len;
    }ws_head;

    char                ws[16];             ///< 发送数据时用来存放包头的
}rn_tcp_ws_t;

typedef struct rn_tcp_t
{
    uint16              alloced;            ///< 是否分配了内存给libuv，且未还回来
    uint16				state;				///< 连接状态 RN_CLOSE
    uint16              mode;               ///< 连接模式（rn_tcp_mode_e）
    uint16				opp_port;			///< 对端port

    int                 tag;                ///< 标识该连接的标签
    int                 timeout;            ///< 超时时间（毫秒）
    int                 wait_time;          ///< 等待心跳包的时间（毫秒）

    rn_tcp_ws_t         *p_tcp_ws;          ///< Websocket的对象
    uv_loop_t           *p_loop;            ///< LibUV的对象指针
    uv_timer_t          *p_timer;           ///< 定时器
    uv_tcp_t		    uv_conn;			///< 连接对象
    
    struct      ///< 用于关闭连接的消息和回调
    {
        tcp_cb_close	    cb;
        void			    *obj;
        uv_async_t          *p_async;
    }close;

    struct      ///< 用于写数据的消息和回调
    {
        uint16              rsv;
        uint16              writting;

        tcp_cb_write	    cb;
        void			    *obj;
        uv_async_t          *p_async;
        uv_write_t          write;
    }write;

    struct      ///< 用于读数据的消息和回调
    {
        uint16              resv;
        uint16              start;

        tcp_cb_alloc	    cb_alloc;
        tcp_cb_read	        cb_read;
        tcp_cb_read_stop    cb_read_stop;

        void			    *obj;
        uv_async_t          *p_async;
        uv_async_t          *p_async_stop;
    }read;

    char				opp_ip[PUB_IP_BUF];			///< 对端ip
}rn_tcp_t;

typedef struct rn_connect_t
{
    uint16              port;
    uint16              rsv;

    int                 tag;
    rn_tcp_t            *p_tcp;

    tcp_cb_connect	    cb;             ///< 建立连接的回调
    void			    *obj;

    uv_connect_t		connect;		///< 连接请求对象

    char				ip[PUB_IP_BUF];			///< 对端ip
}rn_connect_t;

typedef struct rn_client_t
{
    uv_loop_t       *p_loop;

    uv_async_t      async;
    rj_queue_h      will_conn_que;          //rn_connect_t
}rn_client_t;

typedef struct rn_server_t
{
    uint16              listening;
    uint16              port;

    uv_loop_t           *p_loop;
    tcp_cb_accept	    cb;             ///< 建立连接的回调
    void			    *obj;

    uv_tcp_t            listen;			///< 连接对象
    uv_async_t          async;
}rn_server_t;

//////////////////////////////////////////////////////////////////////////
static void cb_timer_close(uv_handle_t* handle)
{
    assert (NULL != handle);
    uv_timer_t *p_timer = (uv_timer_t *)handle->data;
    assert (NULL != p_timer);

    delete p_timer;
}

static void cb_async_close(uv_handle_t* handle)
{
    assert (NULL != handle);
    uv_async_t *p_async = (uv_async_t *)handle->data;
    assert (NULL != p_async);

    delete p_async;
}

static void cb_tcp_close(uv_handle_t* handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);

    //关闭时，可能还有部分没有收取完整的WS包占用上层的内存，需要返回回去。
    if (NULL != p_tcp->p_tcp_ws)
    {
        if ((NULL != p_tcp->read.cb_read) && (NULL != p_tcp->read.obj))
            if (NULL != p_tcp->p_tcp_ws->recv.ed.base)
                p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, 0, &p_tcp->p_tcp_ws->recv.ed);
    }

    if ((NULL != p_tcp->close.cb) && (NULL != p_tcp->close.obj))
    {
        p_tcp->close.cb((rn_tcp_h)p_tcp, p_tcp->close.obj);
    }

    //关闭定时器
    if (NULL != p_tcp->p_timer)
    {
        p_tcp->p_timer->data    = p_tcp->p_timer;
        uv_close((uv_handle_t *)p_tcp->p_timer, cb_timer_close);
    }

    //关闭各种信号
    if (NULL != p_tcp->write.p_async)
    {
        p_tcp->write.p_async->data  = p_tcp->write.p_async;
        uv_close((uv_handle_t *)p_tcp->write.p_async, cb_async_close);
    }

    if (NULL != p_tcp->read.p_async_stop)
    {
        p_tcp->read.p_async_stop->data  = p_tcp->read.p_async_stop;
        uv_close((uv_handle_t *)p_tcp->read.p_async_stop, cb_async_close);
    }

    if (NULL != p_tcp->read.p_async)
    {
        p_tcp->read.p_async->data  = p_tcp->read.p_async;
        uv_close((uv_handle_t *)p_tcp->read.p_async, cb_async_close);
    }

    if (NULL != p_tcp->close.p_async)
    {
        p_tcp->close.p_async->data  = p_tcp->close.p_async;
        uv_close((uv_handle_t *)p_tcp->close.p_async, cb_async_close);
    }

    //删除WS相关东西
    if (NULL != p_tcp->p_tcp_ws)
    {
        if (NULL != p_tcp->p_tcp_ws->cmd.base)
            delete [] p_tcp->p_tcp_ws->cmd.base;

        if (NULL != p_tcp->p_tcp_ws->recv.ing.base)
            delete [] p_tcp->p_tcp_ws->recv.ing.base;

        delete p_tcp->p_tcp_ws;
    }

    delete p_tcp;
}

static void cb_ws_hs(uv_write_t* req, int status)
{
    assert (NULL != req);
    rn_tcp_t *p_tcp = (rn_tcp_t *)req->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->p_tcp_ws->cb) && (NULL != p_tcp->p_tcp_ws->obj));

    p_tcp->wait_time        = 0;

    //若成功，则不需要回调出去，因为需要等待服务端回复。
    if (0 != status)
    {
        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_HS_FAIL);
    }
    else
    {
        if (RN_TCP_SERVER == p_tcp->mode)       //服务端回复成功，则返回握手成功。客户端
        {
            p_tcp->p_tcp_ws->b_hs_ed    = 1;
            p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_OK);

            //重新申请小片内存
            delete [] p_tcp->p_tcp_ws->cmd.base;
            p_tcp->p_tcp_ws->cmd.base   = new char [RN_WS_CMD_BUF_LEN];
            p_tcp->p_tcp_ws->cmd.len    = RN_WS_CMD_BUF_LEN;
        }
        else
        {
            p_tcp->p_tcp_ws->cmd_len    = 0;
            p_tcp->p_tcp_ws->cmd.len    = RN_WS_HS_BUF_LEN;
            memset(p_tcp->p_tcp_ws->cmd.base, 0, RN_WS_HS_BUF_LEN); //把缓存清空，因为马上要用于接收字节，容易出错
        }
    }
}

static void cb_write(uv_write_t* req, int status)
{
    assert (NULL != req);
    rn_tcp_t *p_tcp = (rn_tcp_t *)req->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->write.cb) && (NULL != p_tcp->write.obj));

    //写入已经完成，修改标记
    assert (0 != p_tcp->write.writting);
    p_tcp->write.writting   = 0;
    p_tcp->wait_time        = 0;

    if (0 != status)
    {
        //回调超时
        p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_TIMEOUT);
        p_tcp->state = RN_DISCONNECT;
    }
    else
    {
        //回调成功，可以继续写入数据
        p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_OK);
    }
}

static void cb_read_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_alloc) && (NULL != p_tcp->read.obj));

    //关闭，掉线，都不分配内存。非WS模式的暂停也是如此。
    if ((RN_CLOSE == p_tcp->state) || (RN_DISCONNECT == p_tcp->state) || ((RN_PAUSE == p_tcp->state) && (NULL == p_tcp->p_tcp_ws)))
    {
        buf->base   = NULL;
        buf->len    = 0;

        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//停止读，否则会循环来要求内存，变成死循环。
            p_tcp->read.start       = 0;
        }
    }
    else if ((NULL != p_tcp->p_tcp_ws) && (NULL == p_tcp->p_tcp_ws->recv.ed.base) && (RN_PAUSE == p_tcp->state))
    {
        //WS模式的暂停状态，需要等到(NULL == p_tcp->p_tcp_ws->recv.ed.base)，即收取完整的一个包，才停止分配内存。
        buf->base   = NULL;
        buf->len    = 0;

        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//停止读，否则会循环来要求内存，变成死循环。
            p_tcp->read.start       = 0;
        }
    }
    else
    {
        p_tcp->alloced  = 1;

        if ((NULL != p_tcp->p_tcp_ws) && (0 == p_tcp->p_tcp_ws->b_hs_ed))
        {
            assert (NULL != p_tcp->p_tcp_ws->cmd.base);

            assert (p_tcp->p_tcp_ws->cmd_len < RN_WS_HS_BUF_LEN);

            buf->base   = p_tcp->p_tcp_ws->cmd.base + p_tcp->p_tcp_ws->cmd_len;
            buf->len    = RN_WS_HS_BUF_LEN - p_tcp->p_tcp_ws->cmd_len;
        }
        else if (NULL != p_tcp->p_tcp_ws)
        {
            if (NULL == p_tcp->p_tcp_ws->recv.ing.base)
            {
                assert (0 == p_tcp->p_tcp_ws->recv.head);

                //若接收缓存为空，则分配空间
                p_tcp->p_tcp_ws->recv.ing.base      = new char [RN_WS_CMD_BUF_LEN];
                p_tcp->p_tcp_ws->recv.ing.len       = RN_WS_CMD_BUF_LEN;
            }

            //表示需要接收一个头，则分配自身的内存
            if (0 == p_tcp->p_tcp_ws->recv.head)
            {
                assert (p_tcp->p_tcp_ws->recv.ing_len <= 14);   //可能一个WS头分了多次才收取完成

                buf->base   = p_tcp->p_tcp_ws->recv.ing.base + p_tcp->p_tcp_ws->recv.ing_len;
                buf->len    = p_tcp->p_tcp_ws->recv.ing.len - p_tcp->p_tcp_ws->recv.ing_len;
            }
            else
            {
                //此时说明已经接到一个头，但是尚未部分数据（2）未到

                /*
                1、此时刚刚收完了头加部分数据(1)或者是一个分帧的包部分帧，并且已经向上申请足够的内存准备接收后续的数据。
                2、此时ing中的数据已经复制到ed，但是整个包尚未收取完成。
                3、所以在ed中分配内存给libuv进行收取，不再向上申请新的内存。
                */
                assert (NULL != p_tcp->p_tcp_ws->recv.ed.base);
                assert (p_tcp->p_tcp_ws->ws_head.len <= p_tcp->p_tcp_ws->recv.ed.len);  //不允许出现一个包超出预先分配空间的长度
                assert (p_tcp->p_tcp_ws->recv.ed_len < p_tcp->p_tcp_ws->recv.ed.len);   //不允许出现缓存区已经填满的情况

                //注意：开始位置不要覆盖已经复制的部分数据(A)
                buf->base       = p_tcp->p_tcp_ws->recv.ed.base + p_tcp->p_tcp_ws->recv.ed_len;

                //注意：这里分配的长度为刚好这个包的剩余数据的长度，不可多了，否则下一个包的接收就会出问题
                size_t free_len = p_tcp->p_tcp_ws->ws_head.len - p_tcp->p_tcp_ws->recv.last_len;
                buf->len        = (suggested_size <= free_len) ? suggested_size : free_len;
                assert (0 < free_len);

                //将要收取的数据超出了能分配的内存区，则认为已经出现一个WS包（分帧包）超出了约定的RN_MAX_PACK_LEN。
                if (p_tcp->p_tcp_ws->recv.ed.len < (p_tcp->p_tcp_ws->recv.ed_len + free_len))
                {
                    buf->base   = NULL;     //不分配内存，libuv会在cb_read中回调出错。
                    buf->len    = 0;

                    p_tcp->state    = RN_DISCONNECT;
                }
            }
        }
        else
        {
            p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, suggested_size, buf);
        }
    }
}

static int unpack_ws(rn_tcp_t *p_tcp, size_t data_len, const uv_buf_t* buf)
{
    //若支持加密，送到这里的数据都已经解密
    char *p_base        = buf->base, *p_recved = NULL, fin = 0, op = 0;
    size_t  pack_len = 0, free_len = data_len;
    int  ws_h_len    = 0;

    do 
    {
         ws_h_len = ws_unpack(p_base, (14 < free_len) ? 14 : free_len, &fin, \
             &op, &p_tcp->p_tcp_ws->ws_head.has_key, p_tcp->p_tcp_ws->ws_head.key, &p_tcp->p_tcp_ws->ws_head.len);

         if (ws_h_len < 0)
             return -1;
         
         if (0 == ws_h_len)//一个头都不完整，还需要继续收取，直接返回。
             break;

         //检查数据合法性，防止攻击
         if ((WS_IS_FIN == p_tcp->p_tcp_ws->ws_head.fin) && (WS_CONTINUE != p_tcp->p_tcp_ws->ws_head.opcode) && (WS_CONTINUE == op))
             return -1;

         if ((WS_NO_FIN == p_tcp->p_tcp_ws->ws_head.fin) && (WS_CONTINUE != p_tcp->p_tcp_ws->ws_head.opcode) && \
             ((WS_CONTINUE < op) && (op < WS_CLOSE)) )
             return -1;

         pack_len = p_tcp->p_tcp_ws->ws_head.len + ws_h_len;

         if (((WS_IS_FIN == fin) && (WS_CONTINUE == op)) || (WS_TEXT == op) || (WS_BINARY == op) || (WS_CMD == op))
         {
             p_tcp->p_tcp_ws->ws_head.fin   = fin;

             if ((WS_TEXT == op) || (WS_BINARY == op) || (WS_CMD == op))
                p_tcp->p_tcp_ws->ws_head.opcode = op;

             if ((WS_IS_FIN == fin) && (pack_len <= free_len))
             {
                 /*
                    1、（1）：当前包为最终帧；（2）：当前包已经全部接收到了ing中。
                    2、这种情况下，就需要向上申请内存，准备把数据回调上去。
                    3、每一个分包就回调上去。
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //若没有内存需要申请内存。
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->ws_head.len, &p_tcp->p_tcp_ws->recv.ed);
                     assert (p_tcp->p_tcp_ws->ws_head.len == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 //拼接在原有数据后面
                 char *p_dst = p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len;
                 memcpy(p_dst, p_base+ws_h_len, p_tcp->p_tcp_ws->ws_head.len);
                 p_tcp->p_tcp_ws->recv.ed_len += p_tcp->p_tcp_ws->ws_head.len;
                 assert (p_tcp->p_tcp_ws->recv.ed_len <= p_tcp->p_tcp_ws->recv.ed.len);

                 //若文本，则这里整体解密
                 if ((WS_TEXT == p_tcp->p_tcp_ws->ws_head.opcode) || (WS_CMD == p_tcp->p_tcp_ws->ws_head.opcode))
                 {

                 }

                 //对每个分包进行转码，因为KEY不同
                 if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                 {
                     WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                 }

                 p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->recv.ed_len, &p_tcp->p_tcp_ws->recv.ed);

                 p_tcp->p_tcp_ws->recv.ed.base   = NULL;        //当即释放内存（假动作而已）
                 p_tcp->p_tcp_ws->recv.ed.len    = 0;
                 p_tcp->p_tcp_ws->recv.ed_len    = 0;
                 p_tcp->p_tcp_ws->recv.last_len  = 0;

                 free_len   -= pack_len;
             }
             else if ((WS_IS_FIN == fin) && (free_len < pack_len))
             {
                 /*
                    1、（1）当前包为最终帧；（2）：当前包未能全部接收到了ing中。
                    2、这种情况下，分配一个整包的内存，供后续数据的接收。
                    3、不向上回调，待后续数据收取完整后再回调出去。
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //若没有内存需要申请内存。
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->ws_head.len, &p_tcp->p_tcp_ws->recv.ed);
                     assert (p_tcp->p_tcp_ws->ws_head.len == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 size_t sub_len = free_len - ws_h_len;
                 if (0 < sub_len)   //可能存在刚好收取一个头，数据还未收到的情况，此时(0 == sub_len) 
                 {
                     memcpy(p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len, p_base+ws_h_len, sub_len);
                     p_tcp->p_tcp_ws->recv.ed_len   += sub_len;
                 }

                 free_len                       -= (sub_len + ws_h_len);
                 p_tcp->p_tcp_ws->recv.last_len += sub_len;
                 p_tcp->p_tcp_ws->recv.head      = 1;       //记录收到头，但是数据不完整的情况
             }
             else if (WS_NO_FIN == fin)
             {
                 /*
                    1、当前包为一个多帧包的第一帧。
                    2、这种情况下，分配一个最大的内存块，因为我们约定WS包有一个最大极限值。
                    3、不向上回调，待后续数据收取完整后再回调出去。
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //若没有内存需要申请内存。
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, RN_MAX_PACK_LEN, &p_tcp->p_tcp_ws->recv.ed);
                     assert (RN_MAX_PACK_LEN == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 size_t sub_len = free_len - ws_h_len; //默认为当前分帧尚未收取完成
                 if (pack_len <= free_len)           //当前分帧已经接收完成
                     sub_len = p_tcp->p_tcp_ws->ws_head.len;
                 else
                 {
                     p_tcp->p_tcp_ws->recv.last_len += sub_len;
                     p_tcp->p_tcp_ws->recv.head      = 1;           //记录收到头，但是数据不完整的情况
                 }
                 
                 if (0 < sub_len)   //可能存在刚好收取一个头，数据还未收到的情况，此时(0 == sub_len) 
                 {
                     char *p_dst = p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len;
                     memcpy(p_dst, p_base+ws_h_len, sub_len);
                     p_tcp->p_tcp_ws->recv.ed_len   += sub_len;

                     //对每个分包进行转码，因为KEY不同，要求当前包已经收取完成才转码，没有收取完成的，会在收取完成后转码。
                     if (pack_len <= free_len)
                         if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                         {
                             WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                         }
                 }

                 free_len -= (sub_len + ws_h_len);
             }
             else
             {
                 assert (0);
             }
         }
         else if (WS_CONTINUE == op)
         {
             p_tcp->p_tcp_ws->ws_head.fin   = fin;

             //继续帧，则直接复制数据，检查总长度不能超过缓存区长度就行
             assert (NULL != p_tcp->p_tcp_ws->recv.ed.base);

             size_t sub_len = free_len - ws_h_len; //默认为当前分帧尚未收取完成
             if (pack_len <= free_len)           //当前分帧已经接收完成
                 sub_len = p_tcp->p_tcp_ws->ws_head.len;
             else
             {
                 p_tcp->p_tcp_ws->recv.last_len += sub_len;
                 p_tcp->p_tcp_ws->recv.head      = 1;           //记录收到头，但是数据不完整的情况
             }

             if (0 < sub_len)   //可能存在刚好收取一个头，数据还未收到的情况，此时(0 == sub_len) 
             {
                 memcpy(p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len, p_base+ws_h_len, sub_len);
                 p_tcp->p_tcp_ws->recv.ed_len   += sub_len;
             }

             free_len -= (sub_len + ws_h_len);
         }
         else if (WS_PING == op)
         {
             //回复一个PONG包
             if (pack_len <= free_len)
             {
                 assert (p_tcp == p_tcp->p_tcp_ws->cmd_write.data);
                 assert (p_tcp == p_tcp->uv_conn.data);

                 p_tcp->p_tcp_ws->cmd.base[0] = 0x8A;
                 p_tcp->p_tcp_ws->cmd.base[1] = 0x00;
                 p_tcp->p_tcp_ws->cmd.len     = 2;

                 if(0 != uv_write(&p_tcp->p_tcp_ws->cmd_write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->cmd, 1, NULL))
                 {
                     p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_DISCONNECT);
                 }

                 free_len           -= pack_len;
                 p_tcp->wait_time   = 0;
             }
         }
         else if (WS_PONG == op)       //收到PONG包不需要做任何处理
         {
             if (pack_len <= free_len)
             {
                 free_len           -= pack_len;
                 p_tcp->wait_time   = 0;
             }
         }
         else//WS_CLOSE处理
         {
             if (pack_len <= free_len)
             {
                 p_tcp->state    = RN_CLOSE;
                 p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, &p_tcp->p_tcp_ws->recv.ed);

                 free_len -= pack_len;
             }
         }

         p_base += pack_len;
    } while (0 < free_len);

    if (0 < free_len) //还有数据为处理完成，有一个不完整的头
    {
        //进行一次内存移动，完成刚好ing缓存区已经写到最后的字节，不能再写了。
        //直接在ing内存操作，因为如果是支持加密的话，p_base指向的是解密数据。
        assert (free_len <= 14);
        char *p_src = p_tcp->p_tcp_ws->recv.ing.base + (data_len - free_len);
        memmove(p_tcp->p_tcp_ws->recv.ing.base, p_src, free_len);
    }

    p_tcp->p_tcp_ws->recv.ing_len = free_len;   //

    return 0;
}

static void cb_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    assert (NULL != stream);
    rn_tcp_t *p_tcp = (rn_tcp_t *)stream->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_read) && (NULL != p_tcp->read.obj));

    p_tcp->wait_time        = 0;
    p_tcp->alloced          = 0;

    //出现异常，需要处理
    if (nread < 0)
    {
        if (RN_PAUSE != p_tcp->state)
        {
            if (NULL != p_tcp->p_tcp_ws)
                p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, nread, &p_tcp->p_tcp_ws->recv.ed);
            else
                p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, nread, buf); 
        }
    }
    else if (0 < nread)
    {
        assert ((RN_CLOSE != p_tcp->state) && (RN_DISCONNECT != p_tcp->state));

        if ((NULL != p_tcp->p_tcp_ws) && (0 == p_tcp->p_tcp_ws->b_hs_ed))
        {
            assert ((NULL != p_tcp->p_tcp_ws->cb) && (NULL != p_tcp->p_tcp_ws->obj));

            p_tcp->p_tcp_ws->cmd_len    += nread;
            assert (p_tcp->p_tcp_ws->cmd_len <= p_tcp->p_tcp_ws->cmd.len);
            p_tcp->p_tcp_ws->cmd.base[p_tcp->p_tcp_ws->cmd_len] = '\0'; //把结尾处置为零，保证扫描字符串不会越界出错

            //要检查是否已经收取到了"\r\n"，若没有则继续收取。
            char *p_tail = strstr(p_tcp->p_tcp_ws->cmd.base, "\r\n\r\n");
            if (NULL != p_tail)
            {
                if (RN_TCP_CLIENT == p_tcp->mode)
                {
                    char key [32] = {0};
                    ws_get_value(key, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Accept: ");     //提取服务端回复来的KEY

                    //若相同，则表示身份验证通过，握手成功
                    if (0 == strcmp(key, p_tcp->p_tcp_ws->exp_key))
                    {    
                        p_tcp->p_tcp_ws->b_hs_ed   = 1;
                        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_OK);

                        //握手成功后，检查是否除了握手数据外，还有其他后续数据
                        p_tail += strlen("\r\n\r\n");
                        int free_len = p_tcp->p_tcp_ws->cmd_len - (p_tail - p_tcp->p_tcp_ws->cmd.base);
                        if (0 < free_len)
                        {
                            if (NULL == p_tcp->p_tcp_ws->recv.ing.base)
                            {
                                int ing_len = (RN_WS_CMD_BUF_LEN < free_len) ? (free_len + 16) : RN_WS_CMD_BUF_LEN;
                                p_tcp->p_tcp_ws->recv.ing.base      = new char [ing_len];
                                p_tcp->p_tcp_ws->recv.ing.len       = ing_len;
                            }

                            memcpy(p_tcp->p_tcp_ws->recv.ing.base, p_tail, free_len);
                            p_tcp->p_tcp_ws->recv.ing_len   = free_len;
                        }

                        //重新申请小片内存
                        delete [] p_tcp->p_tcp_ws->cmd.base;
                        p_tcp->p_tcp_ws->cmd.base   = new char [RN_WS_CMD_BUF_LEN];
                        p_tcp->p_tcp_ws->cmd.len    = RN_WS_CMD_BUF_LEN;
                    }
                    else
                    {
                        //非法访问，则主动关闭连接
                        p_tcp->state = RN_CLOSE;

                        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_ILLEGAL);
                    }
                }
                else
                {
                    assert (RN_WS_HS_BUF_LEN == p_tcp->p_tcp_ws->cmd.len);
                    assert (p_tcp->p_tcp_ws->cmd_len == strlen(p_tcp->p_tcp_ws->cmd.base));

                    char str [32] = {0};
                    ws_get_value(str, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Key: ");        //提取客户端发来的KEY
                    ws_encode_key(p_tcp->p_tcp_ws->exp_key, str);                   //加密运算后保存起来，注意发送到客户端

                    //检查协议是否一致
                    ws_get_value(str, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Protocol: ");
                    if (0 == strcmp(str, p_tcp->p_tcp_ws->protocol))
                    {
                        p_tcp->p_tcp_ws->cmd.len    = ws_hs_server(p_tcp->p_tcp_ws->cmd.base, p_tcp->p_tcp_ws->cmd.len, p_tcp->p_tcp_ws->exp_key, p_tcp->p_tcp_ws->protocol); 
                    }
                    else
                    {
                        //回复协议不匹配
                        strcpy(p_tcp->p_tcp_ws->cmd.base, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\nX-WebSocket-Reject-Reason: Client must provide a Host header.");
                        p_tcp->p_tcp_ws->cmd.len = strlen(p_tcp->p_tcp_ws->cmd.base);
                    }

                    assert (p_tcp == p_tcp->p_tcp_ws->cmd_write.data);
                    assert (p_tcp == p_tcp->uv_conn.data);

                    if(0 != uv_write(&p_tcp->p_tcp_ws->cmd_write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->cmd, 1, cb_ws_hs))
                    {
                        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_HS_FAIL);
                    }
                }
            }
        }
        else if (NULL != p_tcp->p_tcp_ws)
        {
            if (0 == p_tcp->p_tcp_ws->recv.head)
            {
                assert (0 == p_tcp->p_tcp_ws->recv.last_len);
                assert (buf->base == (p_tcp->p_tcp_ws->recv.ing.base + p_tcp->p_tcp_ws->recv.ing_len));

                p_tcp->p_tcp_ws->recv.ing_len   += nread;

                //不够一个WS头，则继续收取
                if (2 <= p_tcp->p_tcp_ws->recv.ing_len)
                    if (unpack_ws(p_tcp, p_tcp->p_tcp_ws->recv.ing_len, &p_tcp->p_tcp_ws->recv.ing) < 0)
                    {
                        //非法访问，则主动关闭连接
                        p_tcp->state = RN_CLOSE;
                        p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, &p_tcp->p_tcp_ws->recv.ed);
                    }
            }
            else
            {
                //这里接收一个包的部分数据（B）,整个数据包尚未收取完成，不要向上回调
                assert (buf->base == p_tcp->p_tcp_ws->recv.ed.base + p_tcp->p_tcp_ws->recv.ed_len);

                //则表示当前包以及收取完成，需要准备收取下一个包，如果完成帧，还需要对调上去。
                if (p_tcp->p_tcp_ws->ws_head.len == (nread + p_tcp->p_tcp_ws->recv.last_len))
                {
                    //若是最后一个分帧，则需要向上回调，不是最后分帧，则还需要继续收取，但是依然需要标记下一次要收取头了。
                    if (WS_IS_FIN == p_tcp->p_tcp_ws->ws_head.fin)
                    {
                        //若文本，则这里整体解密
                        if ((WS_TEXT == p_tcp->p_tcp_ws->ws_head.opcode) || (WS_CMD == p_tcp->p_tcp_ws->ws_head.opcode))
                        {

                        }

                        //在此一次性做一次转码运算，姑且假设是先转码再加密的。
                        if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                        {
                            char *p_dst = p_tcp->p_tcp_ws->recv.ed.base + (p_tcp->p_tcp_ws->recv.ed_len - p_tcp->p_tcp_ws->recv.last_len);
                            WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                        }

                        p_tcp->p_tcp_ws->recv.ed_len    += nread;
                        p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->recv.ed_len, &p_tcp->p_tcp_ws->recv.ed);

                        p_tcp->p_tcp_ws->recv.ed.base   = NULL;
                        p_tcp->p_tcp_ws->recv.ed.len    = 0;
                        p_tcp->p_tcp_ws->recv.ed_len    = 0;
                    }
                    else
                    {
                        //一个包收取完成，虽然不是最终分包，还是需要转码
                        if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                        {
                            char *p_dst = p_tcp->p_tcp_ws->recv.ed.base + (p_tcp->p_tcp_ws->recv.ed_len - p_tcp->p_tcp_ws->recv.last_len);
                            WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                        }

                        p_tcp->p_tcp_ws->recv.ed_len    += nread;
                    }

                    //准备好收取下一个包头
                    p_tcp->p_tcp_ws->recv.head      = 0;
                    p_tcp->p_tcp_ws->recv.last_len  = 0;
                }
                else
                {
                    p_tcp->p_tcp_ws->recv.last_len  += nread;
                    p_tcp->p_tcp_ws->recv.ed_len    += nread;
                }
            }
        }
        else
        {
            p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, nread, buf);
        }
    }
}
//////////////////////////////////////////////////////////////////////////

static void on_close(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);

    //若是Websocket模式，则发送关闭帧
    if ((NULL != p_tcp->p_tcp_ws) && (1 == p_tcp->p_tcp_ws->b_hs_ed))
    {
        assert (p_tcp == p_tcp->p_tcp_ws->cmd_write.data);
        assert (p_tcp == p_tcp->uv_conn.data);

        p_tcp->p_tcp_ws->cmd.base[0] = 0x88;
        p_tcp->p_tcp_ws->cmd.base[1] = 0x00;
        p_tcp->p_tcp_ws->cmd.len     = 2;

        uv_write(&p_tcp->p_tcp_ws->cmd_write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->cmd, 1, NULL);
    }

    p_tcp->state    = RN_CLOSE;

    assert (p_tcp == p_tcp->uv_conn.data);
    uv_close((uv_handle_t *)&p_tcp->uv_conn, cb_tcp_close);
}

static void on_write(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);

    if ((NULL != p_tcp->p_tcp_ws) && (0 == p_tcp->p_tcp_ws->b_hs_ed))
    {
        assert ((NULL != p_tcp->p_tcp_ws->cb) && (NULL != p_tcp->p_tcp_ws->obj));

        //WS模式，且未完成握手
        assert (RN_TCP_CLIENT == p_tcp->mode);
        assert (NULL != p_tcp->p_tcp_ws->cmd.base);

        char rand_str [32] = {0};
        ws_rand_key(rand_str, 32);                          //获取一个随机字符串，注意这个需要发送到服务端
        ws_encode_key(p_tcp->p_tcp_ws->exp_key, rand_str);  //保存一段密文，用于和服务端回复比较

        p_tcp->p_tcp_ws->cmd.len = ws_hs_client(p_tcp->p_tcp_ws->cmd.base, p_tcp->p_tcp_ws->cmd.len, rand_str, \
            p_tcp->p_tcp_ws->protocol, p_tcp->opp_ip, p_tcp->opp_port);

        assert (p_tcp == p_tcp->p_tcp_ws->cmd_write.data);
        assert (p_tcp == p_tcp->uv_conn.data);

        if(0 != uv_write(&p_tcp->p_tcp_ws->cmd_write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->cmd, 1, cb_ws_hs))
        {
            p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_HS_FAIL);
        }
    }
    else
    {
        assert ((NULL != p_tcp->write.cb) && (NULL != p_tcp->write.obj));

        //(0 != p_tcp->write.writting)表示正在写数据，则不再回调
        //这样就保证每次调用tcp_cb_write表示上次写数据的结果，而不是其他的。
        //调用tcp_cb_write，在tcp_cb_write中调用rn_tcp_write真正写入数据
        if (0 == p_tcp->write.writting)
            p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_FREE);
    }
}

static void on_read(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_alloc) && (NULL != p_tcp->read.cb_read) && (NULL != p_tcp->read.obj));
    assert (RN_PAUSE == p_tcp->state);

    if (0 != uv_read_start((uv_stream_t *)&p_tcp->uv_conn, cb_read_alloc, cb_read))
    {
        //关闭连接
        p_tcp->state = RN_CLOSE;
        p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, NULL);
    }
    else
    {
        p_tcp->read.start       = 1;

        //成功开启，要修改状态
        p_tcp->state    = RN_CONNECTED;
    }
}

static void on_read_stop(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_read_stop) && (NULL != p_tcp->read.obj));

    //要修改状态，这样在分配那里就不会分配新的内存，而已经分配的还回来后也不能拿到新的内存
    //p_tcp->alloced就会保存在0状态。对libuv的读取停止调用在on_read_alloc中完成。
    p_tcp->state    = RN_PAUSE;

    //分配了内存给了libuv，不能直接停止，而是继续发生信号
    //一个WS包没有收取完成，也不能直接停止
    if ((1 == p_tcp->alloced) || ((NULL != p_tcp->p_tcp_ws) && (NULL != p_tcp->p_tcp_ws->recv.ed.base)))
    {
        uv_async_send(p_tcp->read.p_async_stop);
    }
    else
    {
        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//停止读，否则会循环来要求内存，变成死循环。
            p_tcp->read.start       = 0;
        }

        p_tcp->read.cb_read_stop((rn_tcp_h)p_tcp, p_tcp->read.obj);
    }
}

static void on_timer(uv_timer_t* handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);

    if (RN_CONNECTED == p_tcp->state)
    {
        p_tcp->wait_time    += RN_TIMER_REPEAT;
        if (p_tcp->timeout <= p_tcp->wait_time) //超时
        {
            if ((0 == p_tcp->write.writting) && (0 == p_tcp->alloced))
            {
                if (NULL != p_tcp->p_tcp_ws)
                    p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, &p_tcp->p_tcp_ws->recv.ed);
                else
                    p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, NULL);

                p_tcp->state = RN_CLOSE;
            }
            else
                p_tcp->wait_time    = 0;
        }
        else
        {
            //WS模式的客户端，且已经完成了握手，则发PING包
            if ((NULL != p_tcp->p_tcp_ws) && (RN_TCP_SERVER == p_tcp->mode) && (1 == p_tcp->p_tcp_ws->b_hs_ed))
            {
                assert (p_tcp == p_tcp->write.write.data);
                assert (p_tcp == p_tcp->uv_conn.data);

                p_tcp->p_tcp_ws->cmd.base[0] = 0x89;
                p_tcp->p_tcp_ws->cmd.base[1] = 0x00;
                p_tcp->p_tcp_ws->cmd.len     = 2;

                if(0 != uv_write(&p_tcp->p_tcp_ws->cmd_write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->cmd, 1, NULL))
                {
                    p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_DISCONNECT);
                }
            }

            //让上层准备写一些数据。
            if ((NULL != p_tcp->write.cb) && (NULL != p_tcp->write.obj))
                if (0 == p_tcp->write.writting)
                    p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_FREE);

            //告知上层暂时没有收到数据。
            if ((NULL != p_tcp->read.cb_read) && (NULL != p_tcp->read.obj))
                if (0 == p_tcp->alloced)
                    p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, 0, NULL);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
static void cb_connect(uv_connect_t* req, int status)
{
    rn_connect_t *p_connect = (rn_connect_t *)req->data;
    assert (NULL != p_connect);

    if (status < 0)
    {
       //返回超时
       p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_TIMEOUT);

       //失败，要销毁TCP连接
       uv_close((uv_handle_t *)&p_connect->p_tcp->uv_conn, cb_tcp_close);

       p_connect->p_tcp = NULL;
    }
    else
    {
        //建立连接成功，需要初始化完成整个rn_tcp_conn_t
        rn_tcp_t *p_tcp = p_connect->p_tcp;
        assert (NULL != p_tcp);
        assert (NULL != p_tcp->p_loop);

        p_tcp->state            = RN_PAUSE;
        p_tcp->tag              = p_connect->tag;
        p_tcp->timeout          = RN_TIMEOUT;
        p_tcp->mode             = RN_TCP_CLIENT;
        p_tcp->opp_port         = p_connect->port;
        strcpy(p_tcp->opp_ip, p_connect->ip);

        //初始化信号量
        p_tcp->close.p_async        = new uv_async_t;
        p_tcp->close.p_async->data  = p_tcp;
        uv_async_init(p_tcp->p_loop, p_tcp->close.p_async, on_close);

        p_tcp->write.write.data     = p_tcp;
        p_tcp->write.p_async        = new uv_async_t;
        p_tcp->write.p_async->data  = p_tcp;
        uv_async_init(p_tcp->p_loop, p_tcp->write.p_async, on_write);

        p_tcp->read.p_async         = new uv_async_t;
        p_tcp->read.p_async_stop    = new uv_async_t;
        p_tcp->read.p_async->data           = p_tcp;
        p_tcp->read.p_async_stop->data      = p_tcp;
        uv_async_init(p_tcp->p_loop, p_tcp->read.p_async, on_read);
        uv_async_init(p_tcp->p_loop, p_tcp->read.p_async_stop, on_read_stop);
        
        //初始化定时器
        p_tcp->p_timer              = new uv_timer_t;
        p_tcp->p_timer->data        = p_tcp;
        uv_timer_init(p_tcp->p_loop, p_tcp->p_timer);
        uv_timer_start(p_tcp->p_timer, on_timer, RN_TIMER_TIMEOUT, RN_TIMER_REPEAT);

        //返回成功
        p_connect->cb((rn_tcp_h)p_tcp, p_connect->obj, p_tcp->tag, RN_TCP_OK);
    }

    delete p_connect;
    p_connect = NULL;
}

static void on_connect(uv_async_t * handle)
{
    rn_client_t *p_client = (rn_client_t *)handle->data;
    assert (NULL != p_client);

    struct sockaddr_in addr;

    //遍历队列，把所有的请求发送出去
    while (0 < rj_queue_size(p_client->will_conn_que))
    {
        rn_connect_t *p_connect = (rn_connect_t*)rj_queue_pop_ret(p_client->will_conn_que);

        assert (NULL != p_connect);
        assert (NULL != p_connect->cb);
        assert (NULL != p_connect->obj);
        assert (p_connect->connect.data == p_connect);

        if (0 != uv_ip4_addr(p_connect->ip, p_connect->port, &addr))
        {
            //返回参数错误
            p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_PARAM_ERR);
        }

        //初始化TCP连接
        p_connect->p_tcp    = new rn_tcp_t;
        assert (NULL != p_connect->p_tcp);
        memset(p_connect->p_tcp, 0, sizeof(rn_tcp_t));

        p_connect->p_tcp->p_loop    = p_client->p_loop;
        p_connect->p_tcp->uv_conn.data = p_connect->p_tcp;
        uv_tcp_init(p_client->p_loop, &p_connect->p_tcp->uv_conn);

        //尝试连接服务端
        if (0 != uv_tcp_connect(&p_connect->connect, &p_connect->p_tcp->uv_conn, (const struct sockaddr*)&addr, cb_connect))
        {
            p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_TIMEOUT);

            //失败，要销毁TCP连接
            uv_close((uv_handle_t *)&p_connect->p_tcp->uv_conn, cb_tcp_close);
            
            delete p_connect;
        }
    }
}

static void cb_accept(uv_stream_t* server, int status)
{
    rn_server_t *p_server = (rn_server_t *)server->data;
    assert (NULL != p_server);

    assert (NULL != p_server->cb);
    assert (NULL != p_server->obj);

    if (status < 0)
    {
        //绑定失败，需要关闭
        uv_close((uv_handle_t *)&p_server->listen, NULL);

        //返回失败
        p_server->cb(NULL, p_server->obj);
    }
    else
    {
        //成功则要创建一个rn_tcp_conn_t
        rn_tcp_t *p_tcp    = new rn_tcp_t;
        assert (NULL != p_tcp);
        memset(p_tcp, 0, sizeof(rn_tcp_t));

        p_tcp->p_loop           = p_server->p_loop;
        p_tcp->uv_conn.data     = p_tcp;
        uv_tcp_init(p_tcp->p_loop, &p_tcp->uv_conn);

        if(0 == uv_accept((uv_stream_t *)(&p_server->listen), (uv_stream_t*)(&p_tcp->uv_conn)))
        {
            p_tcp->state    = RN_PAUSE;
            p_tcp->tag      = g_rn_tcp_tag ++;
            p_tcp->timeout  = RN_TIMEOUT;
            p_tcp->mode     = RN_TCP_SERVER;
            p_tcp->opp_port = p_server->port;

            //初始化信号量
            p_tcp->close.p_async        = new uv_async_t;
            p_tcp->close.p_async->data  = p_tcp;
            uv_async_init(p_tcp->p_loop, p_tcp->close.p_async, on_close);

            p_tcp->write.write.data     = p_tcp;
            p_tcp->write.p_async        = new uv_async_t;
            p_tcp->write.p_async->data  = p_tcp;
            uv_async_init(p_tcp->p_loop, p_tcp->write.p_async, on_write);

            p_tcp->read.p_async         = new uv_async_t;
            p_tcp->read.p_async_stop    = new uv_async_t;
            p_tcp->read.p_async->data           = p_tcp;
            p_tcp->read.p_async_stop->data      = p_tcp;
            uv_async_init(p_tcp->p_loop, p_tcp->read.p_async, on_read);
            uv_async_init(p_tcp->p_loop, p_tcp->read.p_async_stop, on_read_stop);

            //初始化定时器
            p_tcp->p_timer              = new uv_timer_t;
            p_tcp->p_timer->data        = p_tcp;
            uv_timer_init(p_tcp->p_loop, p_tcp->p_timer);
            uv_timer_start(p_tcp->p_timer, on_timer, RN_TIMER_TIMEOUT, RN_TIMER_REPEAT);

            //返回成功
            p_server->cb((rn_tcp_h)p_tcp, p_server->obj);
        }
        else
        {
            //关闭连接
            uv_close((uv_handle_t *)(&p_tcp->uv_conn), cb_tcp_close);
        } 
    }
}

static void on_listen(uv_async_t * handle)
{
    rn_server_t *p_server = (rn_server_t *)handle->data;
    assert (NULL != p_server);

    if (0 == p_server->port)
    {
        //停止监听
        uv_close((uv_handle_t *)&p_server->listen, NULL);
    }
    else
    {
        //开启监听
        assert (NULL != p_server->cb);
        assert (NULL != p_server->obj);

        p_server->listen.data   = p_server;
        uv_tcp_init(p_server->p_loop, &p_server->listen);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", p_server->port, &addr);
        if (0 != uv_tcp_bind(&p_server->listen, (const struct sockaddr*)&addr, 0))
        {
            //绑定失败，需要关闭，回调必须为空
            uv_close((uv_handle_t *)&p_server->listen, NULL);

            //返回失败，句柄为空表示监听失败，且已经不能继续监听
            p_server->cb(NULL, p_server->obj);
        }
        else
        {
            if (0 != uv_listen((uv_stream_t *)(&p_server->listen), 1000, cb_accept))
            {
                //监听失败，需要关闭，回调必须为空
                uv_close((uv_handle_t *)&p_server->listen, NULL);

                //返回失败，句柄为空表示监听失败，且已经不能继续监听
                p_server->cb(NULL, p_server->obj);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
rn_client_h rn_client_create(uv_loop_t  *p_loop)
{
    assert (NULL != p_loop);

    if (NULL != p_loop)
    {
        //创建一个用于客服端的对象
        rn_client_t *p_client = new rn_client_t;
        assert (NULL != p_client);
        memset(p_client, 0, sizeof(rn_client_t));

        p_client->p_loop        = p_loop;

        //这个队列用于存放连接申请，每个连接申请进来不是直接调用libuv的连接接口，
        //而是发送一个连接信息，在信号回调函数中再从队列取出所有的请求，依次调用libuv接口建立连接。
        p_client->will_conn_que = rj_queue_create();

        p_client->async.data    = p_client;
        uv_async_init(p_client->p_loop, &p_client->async, on_connect);

        return (rn_client_h)p_client;
    }
    
    return NULL;
}

void rn_client_destroy(rn_client_h loop)
{
    rn_client_t *p_client = (rn_client_t *)loop;
    assert (NULL != p_client);

    //关闭信号，要求此时libuv已经关闭
    uv_close((uv_handle_t *)&p_client->async, NULL);

    //清除请求列表
    while (0 < rj_queue_size(p_client->will_conn_que))
    {
        rn_connect_t *p_connect = (rn_connect_t*)rj_queue_pop_ret(p_client->will_conn_que);

        assert (NULL != p_connect);

        delete p_connect;
    }
    rj_queue_destroy(p_client->will_conn_que);

    //
    delete p_client;
}

rn_server_h rn_server_create(uv_loop_t  *p_loop)
{
    assert (NULL != p_loop);

    if (NULL != p_loop)
    {
        //创建一个用于服务端的对象
        rn_server_t *p_server = new rn_server_t;
        assert (NULL != p_server);
        memset(p_server, 0, sizeof(rn_server_t));

        p_server->p_loop        = p_loop;
        p_server->async.data    = p_server;
        uv_async_init(p_server->p_loop, &p_server->async, on_listen);

        return (rn_server_h)p_server;
    }

    return NULL;
}

void rn_server_destroy(rn_server_h loop)
{
    rn_server_t *p_server = (rn_server_t *)loop;
    assert (NULL != p_server);

    //关闭信号，要求此时libuv已经关闭
    uv_close((uv_handle_t *)&p_server->async, NULL);

    //关闭TCP连接，要求此时libuv已经关闭
    if (0 != p_server->listening)
        uv_close((uv_handle_t *)&p_server->listen, NULL);

    //
    delete p_server;
}

//////////////////////////////////////////////////////////////////////////

int rn_tcp_connect(rn_client_h loop, char *p_ip, uint16 port, tcp_cb_connect cb, void * cb_obj)
{
    assert (NULL != loop);
    assert (NULL != p_ip);
    assert (NULL != cb);
    assert (NULL != cb_obj);

    rn_client_t *p_client = (rn_client_t *)loop;

    if ((NULL != p_client) && (NULL != p_ip) && (NULL != cb) && (NULL != cb_obj))
    {
        //上层应用调用该接口申请建立连接，只是创建一个p_connect对象，初始化必要的参数后
        //送入队列，然后发送一个连接信号给libuv。
        rn_connect_t *p_connect = new rn_connect_t;
        assert (NULL != p_connect);

        memset(p_connect, 0, sizeof(rn_connect_t));

        p_connect->port         = port;
        p_connect->tag          = g_rn_tcp_tag ++;
        p_connect->cb           = cb;
        p_connect->obj          = cb_obj;
        strcpy(p_connect->ip, p_ip);
        p_connect->connect.data = p_connect;

        rj_queue_push(p_client->will_conn_que, p_connect);

        //发送信息
        assert (p_client == p_client->async.data);
        uv_async_send(&p_client->async);

        return p_connect->tag;
    }

    return RN_INVALID_TAG;
}

void rn_tcp_close(rn_tcp_h tcp_h, tcp_cb_close cb, void *cb_obj)
{
    assert (NULL != tcp_h);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if (NULL != p_tcp)
    {
        p_tcp->close.cb     = cb;
        p_tcp->close.obj    = cb_obj;

        assert (p_tcp == p_tcp->close.p_async->data);
        uv_async_send(p_tcp->close.p_async);
    }
}

void rn_tcp_set_timeout(rn_tcp_h tcp_h, int s_timeout)
{
    assert (NULL != tcp_h);
    assert ((0 < s_timeout) && (s_timeout < 300));

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if (NULL != p_tcp)
    {
        p_tcp->timeout  = s_timeout * 1000;
    }
}

int rn_tcp_listen_start(rn_server_h loop, uint16 port, tcp_cb_accept cb, void * cb_obj)
{
    assert (NULL != loop);
    assert (NULL != cb);
    assert (NULL != cb_obj);

    rn_server_t *p_server = (rn_server_t *)loop;

    if ((NULL != p_server) && (NULL != cb) && (NULL != cb_obj))
    {
        p_server->listening  = 1;
        p_server->port  = port;
        p_server->cb    = cb;
        p_server->obj   = cb_obj; 

        //发送信息
        assert (p_server == p_server->async.data);
        uv_async_send(&p_server->async);

        return RN_TCP_OK;
    }

    return RN_TCP_PARAM_ERR;
}

void rn_tcp_listen_stop(rn_server_h loop)
{
    assert (NULL != loop);

    rn_server_t *p_server = (rn_server_t *)loop;

    if (NULL != p_server)
    {
        p_server->listening  = 0; 
        p_server->port  = 0;
        p_server->cb    = NULL;
        p_server->obj   = NULL; 

        //发送信息
        assert (p_server == p_server->async.data);
        uv_async_send(&p_server->async);
    }
}

int rn_tcp_ws_hs(rn_tcp_h tcp_h, char *p_protocol, tcp_ws_cb_hs cb, void *cb_obj)
{
    assert (NULL != tcp_h);
    assert (NULL != p_protocol);
    assert (NULL != cb);
    assert (NULL != cb_obj);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;
    
    if ((NULL != p_tcp) && (NULL != p_protocol) &&(NULL != cb) && (NULL != cb_obj))
    {
        if (RN_CLOSE == p_tcp->state)
            return RN_TCP_DISCONNECT;

        if (RN_DISCONNECT == p_tcp->state)
            return RN_TCP_DISCONNECT;

        assert (NULL == p_tcp->p_tcp_ws);

        p_tcp->p_tcp_ws = new rn_tcp_ws_t;
        assert (NULL != p_tcp->p_tcp_ws);
        memset(p_tcp->p_tcp_ws, 0, sizeof(rn_tcp_ws_t));

        p_tcp->p_tcp_ws->ws_head.fin        = WS_IS_FIN;
        p_tcp->p_tcp_ws->ws_head.opcode     = WS_TEXT;

        p_tcp->p_tcp_ws->b_hs_ed    = 0;
        p_tcp->p_tcp_ws->cmd_write.data = p_tcp;
        p_tcp->p_tcp_ws->cmd.len    = RN_WS_HS_BUF_LEN;
        p_tcp->p_tcp_ws->cmd.base   = new char [RN_WS_HS_BUF_LEN+4];
        memset(p_tcp->p_tcp_ws->cmd.base, 0, RN_WS_HS_BUF_LEN+4);

        strcpy(p_tcp->p_tcp_ws->protocol, p_protocol);

        p_tcp->p_tcp_ws->cb     = cb;
        p_tcp->p_tcp_ws->obj    = cb_obj;

        //若为客户端，则先发起握手，服务端则等待握手
        if (RN_TCP_CLIENT == p_tcp->mode)
        {
            assert (NULL != p_tcp->write.p_async);
            assert (p_tcp == p_tcp->write.p_async->data);
            assert (p_tcp == p_tcp->write.write.data);

            //只是发送一个发送数据的信号，在信号处理对调函数中再实际发送握手数据
            //依靠p_tcp->p_tcp_ws是否为空和握手是否完成来判定
            if (NULL != p_tcp->write.p_async)
                uv_async_send(p_tcp->write.p_async);
        }

        return RN_TCP_OK;
    }

    return RN_TCP_PARAM_ERR;
}
//////////////////////////////////////////////////////////////////////////

int rn_tcp_tag(rn_tcp_h tcp_h)
{
    assert (NULL != tcp_h);
    
    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if (NULL != p_tcp)
    {
        assert (RN_INVALID_TAG != p_tcp->tag);
        return p_tcp->tag;
    }

    return RN_INVALID_TAG;
}

int rn_tcp_try_write(rn_tcp_h tcp_h, tcp_cb_write cb, void *cb_obj)
{
    assert (NULL != tcp_h);
    assert (NULL != cb);
    assert (NULL != cb_obj);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;
    
    if ((NULL != p_tcp) && (NULL != cb) && (NULL != cb_obj))
    {
        if (RN_CLOSE == p_tcp->state)
            return RN_TCP_DISCONNECT;

        if (RN_DISCONNECT == p_tcp->state)
            return RN_TCP_DISCONNECT;

        p_tcp->write.cb             = cb;
        p_tcp->write.obj            = cb_obj;

        assert (NULL != p_tcp->write.p_async);
        assert (p_tcp == p_tcp->write.p_async->data);
        assert (p_tcp == p_tcp->write.write.data);

        if (NULL != p_tcp->write.p_async)
            uv_async_send(p_tcp->write.p_async);

        return RN_TCP_OK;
    }
   
    return RN_TCP_PARAM_ERR;
}

/*
这个借口在libuv线程调用，可以直接调用libuv的接口
在Websocket模式下，约定在p_data.base始终指向的数据格式为：rn_ws_packet_t + data。
*/
int rn_tcp_write(rn_tcp_h tcp_h, uv_buf_t * p_data)
{
    assert (NULL != tcp_h);
    assert (NULL != p_data);
    
    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if ((NULL != p_tcp) && (NULL != p_data))
    {
        assert (p_tcp == p_tcp->write.write.data);
        assert (p_tcp == p_tcp->uv_conn.data);
        assert (0 == p_tcp->write.writting);

        p_tcp->write.writting   = 1;

        if (NULL != p_tcp->p_tcp_ws)
        {
            assert (0 != p_tcp->p_tcp_ws->b_hs_ed);
            uint16 ws_h_len = 0;
            rn_ws_packet_t *p_ws_t = NULL;

            p_ws_t = (rn_ws_packet_t *)p_data->base;

            p_data->len -= sizeof(rn_ws_packet_t);    //修正长度
            assert ((0 < p_data->len) && (p_data->len <= RN_MAX_PACK_LEN));

            ws_h_len = ws_pack(p_tcp->p_tcp_ws->ws, WS_IS_FIN, p_ws_t->type, 0, NULL, p_data->len);

            p_tcp->p_tcp_ws->write_buf.base  = p_data->base + (sizeof(rn_ws_packet_t) - ws_h_len);
            p_tcp->p_tcp_ws->write_buf.len   = p_data->len + ws_h_len;

            if ((WS_TEXT == p_ws_t->type) || (WS_CMD == p_ws_t->type))
            {
                memcpy(p_tcp->p_tcp_ws->write_buf.base, p_tcp->p_tcp_ws->ws, ws_h_len);

                if(0 != uv_write(&p_tcp->write.write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->write_buf, 1, cb_write))
                    return RN_TCP_TIMEOUT;
            }
            else
            {
                memcpy(p_tcp->p_tcp_ws->write_buf.base, p_tcp->p_tcp_ws->ws, ws_h_len);

                if(0 != uv_write(&p_tcp->write.write, (uv_stream_t*)&p_tcp->uv_conn, &p_tcp->p_tcp_ws->write_buf, 1, cb_write))
                    return RN_TCP_TIMEOUT;
            }
        }
        else
        {
            if(0 != uv_write(&p_tcp->write.write, (uv_stream_t*)&p_tcp->uv_conn, p_data, 1, cb_write))
                return RN_TCP_TIMEOUT;
        }

        return RN_TCP_OK;
    }

    return RN_TCP_PARAM_ERR;
}

int rn_tcp_read_start(rn_tcp_h tcp_h, tcp_cb_alloc cb_alloc, tcp_cb_read cb_read, void *cb_obj)
{
    assert (NULL != tcp_h);
    assert (NULL != cb_alloc);
    assert (NULL != cb_read);
    assert (NULL != cb_obj);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if ((NULL != p_tcp) && (NULL != cb_alloc) && (NULL != cb_read) && (NULL != cb_obj))
    {
        if (RN_CLOSE == p_tcp->state)
            return RN_TCP_DISCONNECT;

        if (RN_DISCONNECT == p_tcp->state)
            return RN_TCP_DISCONNECT;

        p_tcp->read.cb_alloc        = cb_alloc;
        p_tcp->read.cb_read         = cb_read;
        p_tcp->read.obj             = cb_obj;

        assert (NULL != p_tcp->read.p_async);
        assert (p_tcp->read.p_async->data == p_tcp);
        
        if (NULL != p_tcp->read.p_async)
            uv_async_send(p_tcp->read.p_async);

        return RN_TCP_OK;
    }

    return RN_TCP_PARAM_ERR;
}

void rn_tcp_read_stop(rn_tcp_h tcp_h, tcp_cb_read_stop cb_read_stop, void *cb_obj)
{
    assert (NULL != tcp_h);
    assert (NULL != cb_read_stop);
    assert (NULL != cb_obj);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if ((NULL != p_tcp) && (NULL != cb_read_stop) && (NULL != cb_obj))
    {
        if (RN_CLOSE == p_tcp->state)
            return ;

        if (RN_DISCONNECT == p_tcp->state)
            return ;

        p_tcp->read.cb_read_stop    = cb_read_stop;
        p_tcp->read.obj             = cb_obj;

        assert (NULL != p_tcp->read.p_async_stop);
        assert (p_tcp->read.p_async_stop->data == p_tcp);
        
        if (NULL != p_tcp->read.p_async_stop)
            uv_async_send(p_tcp->read.p_async_stop);
    }
}

int rn_tcp_state(rn_tcp_h tcp_h)
{
    assert (NULL != tcp_h);

    rn_tcp_t *p_tcp = (rn_tcp_t *)tcp_h;

    if (NULL != p_tcp)
    {
        return (RN_PAUSE == p_tcp->state) ? RN_CONNECTED : p_tcp->state;
    }

    return RN_CLOSE;
}
//end


