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

const   int     RN_TIMEOUT          = 10000;//10��

int     g_rn_tcp_tag                = 100;

typedef enum rn_tcp_mode_e
{
    RN_TCP_SERVER       = 0,
    RN_TCP_CLIENT
}rn_tcp_mode_e;

typedef struct rn_tcp_ws_t
{
    uint16              b_hs_ed;        ///< �Ƿ��������
    uint16              rsv;

    tcp_ws_cb_hs        cb;             ///< ���ֻص�
    void                *obj;           ///< ���������ָ��

    size_t              cmd_len;        ///< �Ѿ���ȡ�Ŀ��ƻ����е����ݳ���
    uv_buf_t            cmd;            ///< ר�����ڷ��Ϳ��������
    uv_write_t          cmd_write;      ///< ר�����ڷ��Ϳ�������
    uv_buf_t            write_buf;      ///< ���ڷ�������

    char                exp_key[32];    ///< ����Э�̵���Կ
    char                protocol[32];   ///< ��Э��

    struct 
    {
        uint16              rsv;
        uint16              head;           ///< �Ƿ�ӵ���һ��WSͷ��0��ʾû��ͷ
        
        size_t              last_len;       ///< ���һ�����Ѿ���ȡ�ĳ���
        size_t              ing_len;        ///< ���ݽ���λ��
        size_t              ed_len;         ///< ��������ҽ��ܵ����ݳ���
        uv_buf_t            ing;            ///< ���ڽ��յĻ�����
        uv_buf_t            ed;             ///< �Ѿ����ܺ������
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

    char                ws[16];             ///< ��������ʱ������Ű�ͷ��
}rn_tcp_ws_t;

typedef struct rn_tcp_t
{
    uint16              alloced;            ///< �Ƿ�������ڴ��libuv����δ������
    uint16				state;				///< ����״̬ RN_CLOSE
    uint16              mode;               ///< ����ģʽ��rn_tcp_mode_e��
    uint16				opp_port;			///< �Զ�port

    int                 tag;                ///< ��ʶ�����ӵı�ǩ
    int                 timeout;            ///< ��ʱʱ�䣨���룩
    int                 wait_time;          ///< �ȴ���������ʱ�䣨���룩

    rn_tcp_ws_t         *p_tcp_ws;          ///< Websocket�Ķ���
    uv_loop_t           *p_loop;            ///< LibUV�Ķ���ָ��
    uv_timer_t          *p_timer;           ///< ��ʱ��
    uv_tcp_t		    uv_conn;			///< ���Ӷ���
    
    struct      ///< ���ڹر����ӵ���Ϣ�ͻص�
    {
        tcp_cb_close	    cb;
        void			    *obj;
        uv_async_t          *p_async;
    }close;

    struct      ///< ����д���ݵ���Ϣ�ͻص�
    {
        uint16              rsv;
        uint16              writting;

        tcp_cb_write	    cb;
        void			    *obj;
        uv_async_t          *p_async;
        uv_write_t          write;
    }write;

    struct      ///< ���ڶ����ݵ���Ϣ�ͻص�
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

    char				opp_ip[PUB_IP_BUF];			///< �Զ�ip
}rn_tcp_t;

typedef struct rn_connect_t
{
    uint16              port;
    uint16              rsv;

    int                 tag;
    rn_tcp_t            *p_tcp;

    tcp_cb_connect	    cb;             ///< �������ӵĻص�
    void			    *obj;

    uv_connect_t		connect;		///< �����������

    char				ip[PUB_IP_BUF];			///< �Զ�ip
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
    tcp_cb_accept	    cb;             ///< �������ӵĻص�
    void			    *obj;

    uv_tcp_t            listen;			///< ���Ӷ���
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

    //�ر�ʱ�����ܻ��в���û����ȡ������WS��ռ���ϲ���ڴ棬��Ҫ���ػ�ȥ��
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

    //�رն�ʱ��
    if (NULL != p_tcp->p_timer)
    {
        p_tcp->p_timer->data    = p_tcp->p_timer;
        uv_close((uv_handle_t *)p_tcp->p_timer, cb_timer_close);
    }

    //�رո����ź�
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

    //ɾ��WS��ض���
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

    //���ɹ�������Ҫ�ص���ȥ����Ϊ��Ҫ�ȴ�����˻ظ���
    if (0 != status)
    {
        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_HS_FAIL);
    }
    else
    {
        if (RN_TCP_SERVER == p_tcp->mode)       //����˻ظ��ɹ����򷵻����ֳɹ����ͻ���
        {
            p_tcp->p_tcp_ws->b_hs_ed    = 1;
            p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_OK);

            //��������СƬ�ڴ�
            delete [] p_tcp->p_tcp_ws->cmd.base;
            p_tcp->p_tcp_ws->cmd.base   = new char [RN_WS_CMD_BUF_LEN];
            p_tcp->p_tcp_ws->cmd.len    = RN_WS_CMD_BUF_LEN;
        }
        else
        {
            p_tcp->p_tcp_ws->cmd_len    = 0;
            p_tcp->p_tcp_ws->cmd.len    = RN_WS_HS_BUF_LEN;
            memset(p_tcp->p_tcp_ws->cmd.base, 0, RN_WS_HS_BUF_LEN); //�ѻ�����գ���Ϊ����Ҫ���ڽ����ֽڣ����׳���
        }
    }
}

static void cb_write(uv_write_t* req, int status)
{
    assert (NULL != req);
    rn_tcp_t *p_tcp = (rn_tcp_t *)req->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->write.cb) && (NULL != p_tcp->write.obj));

    //д���Ѿ���ɣ��޸ı��
    assert (0 != p_tcp->write.writting);
    p_tcp->write.writting   = 0;
    p_tcp->wait_time        = 0;

    if (0 != status)
    {
        //�ص���ʱ
        p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_TIMEOUT);
        p_tcp->state = RN_DISCONNECT;
    }
    else
    {
        //�ص��ɹ������Լ���д������
        p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_OK);
    }
}

static void cb_read_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_alloc) && (NULL != p_tcp->read.obj));

    //�رգ����ߣ����������ڴ档��WSģʽ����ͣҲ����ˡ�
    if ((RN_CLOSE == p_tcp->state) || (RN_DISCONNECT == p_tcp->state) || ((RN_PAUSE == p_tcp->state) && (NULL == p_tcp->p_tcp_ws)))
    {
        buf->base   = NULL;
        buf->len    = 0;

        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//ֹͣ���������ѭ����Ҫ���ڴ棬�����ѭ����
            p_tcp->read.start       = 0;
        }
    }
    else if ((NULL != p_tcp->p_tcp_ws) && (NULL == p_tcp->p_tcp_ws->recv.ed.base) && (RN_PAUSE == p_tcp->state))
    {
        //WSģʽ����ͣ״̬����Ҫ�ȵ�(NULL == p_tcp->p_tcp_ws->recv.ed.base)������ȡ������һ��������ֹͣ�����ڴ档
        buf->base   = NULL;
        buf->len    = 0;

        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//ֹͣ���������ѭ����Ҫ���ڴ棬�����ѭ����
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

                //�����ջ���Ϊ�գ������ռ�
                p_tcp->p_tcp_ws->recv.ing.base      = new char [RN_WS_CMD_BUF_LEN];
                p_tcp->p_tcp_ws->recv.ing.len       = RN_WS_CMD_BUF_LEN;
            }

            //��ʾ��Ҫ����һ��ͷ�������������ڴ�
            if (0 == p_tcp->p_tcp_ws->recv.head)
            {
                assert (p_tcp->p_tcp_ws->recv.ing_len <= 14);   //����һ��WSͷ���˶�β���ȡ���

                buf->base   = p_tcp->p_tcp_ws->recv.ing.base + p_tcp->p_tcp_ws->recv.ing_len;
                buf->len    = p_tcp->p_tcp_ws->recv.ing.len - p_tcp->p_tcp_ws->recv.ing_len;
            }
            else
            {
                //��ʱ˵���Ѿ��ӵ�һ��ͷ��������δ�������ݣ�2��δ��

                /*
                1����ʱ�ո�������ͷ�Ӳ�������(1)������һ����֡�İ�����֡�������Ѿ����������㹻���ڴ�׼�����պ��������ݡ�
                2����ʱing�е������Ѿ����Ƶ�ed��������������δ��ȡ��ɡ�
                3��������ed�з����ڴ��libuv������ȡ���������������µ��ڴ档
                */
                assert (NULL != p_tcp->p_tcp_ws->recv.ed.base);
                assert (p_tcp->p_tcp_ws->ws_head.len <= p_tcp->p_tcp_ws->recv.ed.len);  //���������һ��������Ԥ�ȷ���ռ�ĳ���
                assert (p_tcp->p_tcp_ws->recv.ed_len < p_tcp->p_tcp_ws->recv.ed.len);   //��������ֻ������Ѿ����������

                //ע�⣺��ʼλ�ò�Ҫ�����Ѿ����ƵĲ�������(A)
                buf->base       = p_tcp->p_tcp_ws->recv.ed.base + p_tcp->p_tcp_ws->recv.ed_len;

                //ע�⣺�������ĳ���Ϊ�պ��������ʣ�����ݵĳ��ȣ����ɶ��ˣ�������һ�����Ľ��վͻ������
                size_t free_len = p_tcp->p_tcp_ws->ws_head.len - p_tcp->p_tcp_ws->recv.last_len;
                buf->len        = (suggested_size <= free_len) ? suggested_size : free_len;
                assert (0 < free_len);

                //��Ҫ��ȡ�����ݳ������ܷ�����ڴ���������Ϊ�Ѿ�����һ��WS������֡����������Լ����RN_MAX_PACK_LEN��
                if (p_tcp->p_tcp_ws->recv.ed.len < (p_tcp->p_tcp_ws->recv.ed_len + free_len))
                {
                    buf->base   = NULL;     //�������ڴ棬libuv����cb_read�лص�����
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
    //��֧�ּ��ܣ��͵���������ݶ��Ѿ�����
    char *p_base        = buf->base, *p_recved = NULL, fin = 0, op = 0;
    size_t  pack_len = 0, free_len = data_len;
    int  ws_h_len    = 0;

    do 
    {
         ws_h_len = ws_unpack(p_base, (14 < free_len) ? 14 : free_len, &fin, \
             &op, &p_tcp->p_tcp_ws->ws_head.has_key, p_tcp->p_tcp_ws->ws_head.key, &p_tcp->p_tcp_ws->ws_head.len);

         if (ws_h_len < 0)
             return -1;
         
         if (0 == ws_h_len)//һ��ͷ��������������Ҫ������ȡ��ֱ�ӷ��ء�
             break;

         //������ݺϷ��ԣ���ֹ����
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
                    1����1������ǰ��Ϊ����֡����2������ǰ���Ѿ�ȫ�����յ���ing�С�
                    2����������£�����Ҫ���������ڴ棬׼�������ݻص���ȥ��
                    3��ÿһ���ְ��ͻص���ȥ��
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //��û���ڴ���Ҫ�����ڴ档
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->ws_head.len, &p_tcp->p_tcp_ws->recv.ed);
                     assert (p_tcp->p_tcp_ws->ws_head.len == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 //ƴ����ԭ�����ݺ���
                 char *p_dst = p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len;
                 memcpy(p_dst, p_base+ws_h_len, p_tcp->p_tcp_ws->ws_head.len);
                 p_tcp->p_tcp_ws->recv.ed_len += p_tcp->p_tcp_ws->ws_head.len;
                 assert (p_tcp->p_tcp_ws->recv.ed_len <= p_tcp->p_tcp_ws->recv.ed.len);

                 //���ı����������������
                 if ((WS_TEXT == p_tcp->p_tcp_ws->ws_head.opcode) || (WS_CMD == p_tcp->p_tcp_ws->ws_head.opcode))
                 {

                 }

                 //��ÿ���ְ�����ת�룬��ΪKEY��ͬ
                 if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                 {
                     WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                 }

                 p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->recv.ed_len, &p_tcp->p_tcp_ws->recv.ed);

                 p_tcp->p_tcp_ws->recv.ed.base   = NULL;        //�����ͷ��ڴ棨�ٶ������ѣ�
                 p_tcp->p_tcp_ws->recv.ed.len    = 0;
                 p_tcp->p_tcp_ws->recv.ed_len    = 0;
                 p_tcp->p_tcp_ws->recv.last_len  = 0;

                 free_len   -= pack_len;
             }
             else if ((WS_IS_FIN == fin) && (free_len < pack_len))
             {
                 /*
                    1����1����ǰ��Ϊ����֡����2������ǰ��δ��ȫ�����յ���ing�С�
                    2����������£�����һ���������ڴ棬���������ݵĽ��ա�
                    3�������ϻص���������������ȡ�������ٻص���ȥ��
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //��û���ڴ���Ҫ�����ڴ档
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, p_tcp->p_tcp_ws->ws_head.len, &p_tcp->p_tcp_ws->recv.ed);
                     assert (p_tcp->p_tcp_ws->ws_head.len == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 size_t sub_len = free_len - ws_h_len;
                 if (0 < sub_len)   //���ܴ��ڸպ���ȡһ��ͷ�����ݻ�δ�յ����������ʱ(0 == sub_len) 
                 {
                     memcpy(p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len, p_base+ws_h_len, sub_len);
                     p_tcp->p_tcp_ws->recv.ed_len   += sub_len;
                 }

                 free_len                       -= (sub_len + ws_h_len);
                 p_tcp->p_tcp_ws->recv.last_len += sub_len;
                 p_tcp->p_tcp_ws->recv.head      = 1;       //��¼�յ�ͷ���������ݲ����������
             }
             else if (WS_NO_FIN == fin)
             {
                 /*
                    1����ǰ��Ϊһ����֡���ĵ�һ֡��
                    2����������£�����һ�������ڴ�飬��Ϊ����Լ��WS����һ�������ֵ��
                    3�������ϻص���������������ȡ�������ٻص���ȥ��
                */
                 if (NULL == p_tcp->p_tcp_ws->recv.ed.base)     //��û���ڴ���Ҫ�����ڴ档
                 {
                     assert (0 == p_tcp->p_tcp_ws->recv.ed_len);
                     p_tcp->read.cb_alloc((rn_tcp_h)p_tcp, p_tcp->read.obj, RN_MAX_PACK_LEN, &p_tcp->p_tcp_ws->recv.ed);
                     assert (RN_MAX_PACK_LEN == p_tcp->p_tcp_ws->recv.ed.len);
                 }

                 size_t sub_len = free_len - ws_h_len; //Ĭ��Ϊ��ǰ��֡��δ��ȡ���
                 if (pack_len <= free_len)           //��ǰ��֡�Ѿ��������
                     sub_len = p_tcp->p_tcp_ws->ws_head.len;
                 else
                 {
                     p_tcp->p_tcp_ws->recv.last_len += sub_len;
                     p_tcp->p_tcp_ws->recv.head      = 1;           //��¼�յ�ͷ���������ݲ����������
                 }
                 
                 if (0 < sub_len)   //���ܴ��ڸպ���ȡһ��ͷ�����ݻ�δ�յ����������ʱ(0 == sub_len) 
                 {
                     char *p_dst = p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len;
                     memcpy(p_dst, p_base+ws_h_len, sub_len);
                     p_tcp->p_tcp_ws->recv.ed_len   += sub_len;

                     //��ÿ���ְ�����ת�룬��ΪKEY��ͬ��Ҫ��ǰ���Ѿ���ȡ��ɲ�ת�룬û����ȡ��ɵģ�������ȡ��ɺ�ת�롣
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

             //����֡����ֱ�Ӹ������ݣ�����ܳ��Ȳ��ܳ������������Ⱦ���
             assert (NULL != p_tcp->p_tcp_ws->recv.ed.base);

             size_t sub_len = free_len - ws_h_len; //Ĭ��Ϊ��ǰ��֡��δ��ȡ���
             if (pack_len <= free_len)           //��ǰ��֡�Ѿ��������
                 sub_len = p_tcp->p_tcp_ws->ws_head.len;
             else
             {
                 p_tcp->p_tcp_ws->recv.last_len += sub_len;
                 p_tcp->p_tcp_ws->recv.head      = 1;           //��¼�յ�ͷ���������ݲ����������
             }

             if (0 < sub_len)   //���ܴ��ڸպ���ȡһ��ͷ�����ݻ�δ�յ����������ʱ(0 == sub_len) 
             {
                 memcpy(p_tcp->p_tcp_ws->recv.ed.base+p_tcp->p_tcp_ws->recv.ed_len, p_base+ws_h_len, sub_len);
                 p_tcp->p_tcp_ws->recv.ed_len   += sub_len;
             }

             free_len -= (sub_len + ws_h_len);
         }
         else if (WS_PING == op)
         {
             //�ظ�һ��PONG��
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
         else if (WS_PONG == op)       //�յ�PONG������Ҫ���κδ���
         {
             if (pack_len <= free_len)
             {
                 free_len           -= pack_len;
                 p_tcp->wait_time   = 0;
             }
         }
         else//WS_CLOSE����
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

    if (0 < free_len) //��������Ϊ������ɣ���һ����������ͷ
    {
        //����һ���ڴ��ƶ�����ɸպ�ing�������Ѿ�д�������ֽڣ�������д�ˡ�
        //ֱ����ing�ڴ��������Ϊ�����֧�ּ��ܵĻ���p_baseָ����ǽ������ݡ�
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

    //�����쳣����Ҫ����
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
            p_tcp->p_tcp_ws->cmd.base[p_tcp->p_tcp_ws->cmd_len] = '\0'; //�ѽ�β����Ϊ�㣬��֤ɨ���ַ�������Խ�����

            //Ҫ����Ƿ��Ѿ���ȡ����"\r\n"����û���������ȡ��
            char *p_tail = strstr(p_tcp->p_tcp_ws->cmd.base, "\r\n\r\n");
            if (NULL != p_tail)
            {
                if (RN_TCP_CLIENT == p_tcp->mode)
                {
                    char key [32] = {0};
                    ws_get_value(key, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Accept: ");     //��ȡ����˻ظ�����KEY

                    //����ͬ�����ʾ�����֤ͨ�������ֳɹ�
                    if (0 == strcmp(key, p_tcp->p_tcp_ws->exp_key))
                    {    
                        p_tcp->p_tcp_ws->b_hs_ed   = 1;
                        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_OK);

                        //���ֳɹ��󣬼���Ƿ�������������⣬����������������
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

                        //��������СƬ�ڴ�
                        delete [] p_tcp->p_tcp_ws->cmd.base;
                        p_tcp->p_tcp_ws->cmd.base   = new char [RN_WS_CMD_BUF_LEN];
                        p_tcp->p_tcp_ws->cmd.len    = RN_WS_CMD_BUF_LEN;
                    }
                    else
                    {
                        //�Ƿ����ʣ��������ر�����
                        p_tcp->state = RN_CLOSE;

                        p_tcp->p_tcp_ws->cb((rn_tcp_h)p_tcp, p_tcp->p_tcp_ws->obj, RN_TCP_ILLEGAL);
                    }
                }
                else
                {
                    assert (RN_WS_HS_BUF_LEN == p_tcp->p_tcp_ws->cmd.len);
                    assert (p_tcp->p_tcp_ws->cmd_len == strlen(p_tcp->p_tcp_ws->cmd.base));

                    char str [32] = {0};
                    ws_get_value(str, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Key: ");        //��ȡ�ͻ��˷�����KEY
                    ws_encode_key(p_tcp->p_tcp_ws->exp_key, str);                   //��������󱣴�������ע�ⷢ�͵��ͻ���

                    //���Э���Ƿ�һ��
                    ws_get_value(str, 32, p_tcp->p_tcp_ws->cmd.base, "Sec-WebSocket-Protocol: ");
                    if (0 == strcmp(str, p_tcp->p_tcp_ws->protocol))
                    {
                        p_tcp->p_tcp_ws->cmd.len    = ws_hs_server(p_tcp->p_tcp_ws->cmd.base, p_tcp->p_tcp_ws->cmd.len, p_tcp->p_tcp_ws->exp_key, p_tcp->p_tcp_ws->protocol); 
                    }
                    else
                    {
                        //�ظ�Э�鲻ƥ��
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

                //����һ��WSͷ���������ȡ
                if (2 <= p_tcp->p_tcp_ws->recv.ing_len)
                    if (unpack_ws(p_tcp, p_tcp->p_tcp_ws->recv.ing_len, &p_tcp->p_tcp_ws->recv.ing) < 0)
                    {
                        //�Ƿ����ʣ��������ر�����
                        p_tcp->state = RN_CLOSE;
                        p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, &p_tcp->p_tcp_ws->recv.ed);
                    }
            }
            else
            {
                //�������һ�����Ĳ������ݣ�B��,�������ݰ���δ��ȡ��ɣ���Ҫ���ϻص�
                assert (buf->base == p_tcp->p_tcp_ws->recv.ed.base + p_tcp->p_tcp_ws->recv.ed_len);

                //���ʾ��ǰ���Լ���ȡ��ɣ���Ҫ׼����ȡ��һ������������֡������Ҫ�Ե���ȥ��
                if (p_tcp->p_tcp_ws->ws_head.len == (nread + p_tcp->p_tcp_ws->recv.last_len))
                {
                    //�������һ����֡������Ҫ���ϻص�����������֡������Ҫ������ȡ��������Ȼ��Ҫ�����һ��Ҫ��ȡͷ�ˡ�
                    if (WS_IS_FIN == p_tcp->p_tcp_ws->ws_head.fin)
                    {
                        //���ı����������������
                        if ((WS_TEXT == p_tcp->p_tcp_ws->ws_head.opcode) || (WS_CMD == p_tcp->p_tcp_ws->ws_head.opcode))
                        {

                        }

                        //�ڴ�һ������һ��ת�����㣬���Ҽ�������ת���ټ��ܵġ�
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
                        //һ������ȡ��ɣ���Ȼ�������շְ���������Ҫת��
                        if (0 != p_tcp->p_tcp_ws->ws_head.has_key)
                        {
                            char *p_dst = p_tcp->p_tcp_ws->recv.ed.base + (p_tcp->p_tcp_ws->recv.ed_len - p_tcp->p_tcp_ws->recv.last_len);
                            WS_UN_MASK(p_dst, p_tcp->p_tcp_ws->ws_head.key, p_tcp->p_tcp_ws->ws_head.len)
                        }

                        p_tcp->p_tcp_ws->recv.ed_len    += nread;
                    }

                    //׼������ȡ��һ����ͷ
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

    //����Websocketģʽ�����͹ر�֡
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

        //WSģʽ����δ�������
        assert (RN_TCP_CLIENT == p_tcp->mode);
        assert (NULL != p_tcp->p_tcp_ws->cmd.base);

        char rand_str [32] = {0};
        ws_rand_key(rand_str, 32);                          //��ȡһ������ַ�����ע�������Ҫ���͵������
        ws_encode_key(p_tcp->p_tcp_ws->exp_key, rand_str);  //����һ�����ģ����ںͷ���˻ظ��Ƚ�

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

        //(0 != p_tcp->write.writting)��ʾ����д���ݣ����ٻص�
        //�����ͱ�֤ÿ�ε���tcp_cb_write��ʾ�ϴ�д���ݵĽ���������������ġ�
        //����tcp_cb_write����tcp_cb_write�е���rn_tcp_write����д������
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
        //�ر�����
        p_tcp->state = RN_CLOSE;
        p_tcp->read.cb_read((rn_tcp_h)p_tcp, p_tcp->read.obj, -1, NULL);
    }
    else
    {
        p_tcp->read.start       = 1;

        //�ɹ�������Ҫ�޸�״̬
        p_tcp->state    = RN_CONNECTED;
    }
}

static void on_read_stop(uv_async_t * handle)
{
    assert (NULL != handle);
    rn_tcp_t *p_tcp = (rn_tcp_t *)handle->data;
    assert (NULL != p_tcp);
    assert ((NULL != p_tcp->read.cb_read_stop) && (NULL != p_tcp->read.obj));

    //Ҫ�޸�״̬�������ڷ�������Ͳ�������µ��ڴ棬���Ѿ�����Ļ�������Ҳ�����õ��µ��ڴ�
    //p_tcp->alloced�ͻᱣ����0״̬����libuv�Ķ�ȡֹͣ������on_read_alloc����ɡ�
    p_tcp->state    = RN_PAUSE;

    //�������ڴ����libuv������ֱ��ֹͣ�����Ǽ��������ź�
    //һ��WS��û����ȡ��ɣ�Ҳ����ֱ��ֹͣ
    if ((1 == p_tcp->alloced) || ((NULL != p_tcp->p_tcp_ws) && (NULL != p_tcp->p_tcp_ws->recv.ed.base)))
    {
        uv_async_send(p_tcp->read.p_async_stop);
    }
    else
    {
        if (1 == p_tcp->read.start)
        {
            uv_read_stop((uv_stream_t *)&p_tcp->uv_conn);//ֹͣ���������ѭ����Ҫ���ڴ棬�����ѭ����
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
        if (p_tcp->timeout <= p_tcp->wait_time) //��ʱ
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
            //WSģʽ�Ŀͻ��ˣ����Ѿ���������֣���PING��
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

            //���ϲ�׼��дһЩ���ݡ�
            if ((NULL != p_tcp->write.cb) && (NULL != p_tcp->write.obj))
                if (0 == p_tcp->write.writting)
                    p_tcp->write.cb((rn_tcp_h)p_tcp, p_tcp->write.obj, RN_TCP_FREE);

            //��֪�ϲ���ʱû���յ����ݡ�
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
       //���س�ʱ
       p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_TIMEOUT);

       //ʧ�ܣ�Ҫ����TCP����
       uv_close((uv_handle_t *)&p_connect->p_tcp->uv_conn, cb_tcp_close);

       p_connect->p_tcp = NULL;
    }
    else
    {
        //�������ӳɹ�����Ҫ��ʼ���������rn_tcp_conn_t
        rn_tcp_t *p_tcp = p_connect->p_tcp;
        assert (NULL != p_tcp);
        assert (NULL != p_tcp->p_loop);

        p_tcp->state            = RN_PAUSE;
        p_tcp->tag              = p_connect->tag;
        p_tcp->timeout          = RN_TIMEOUT;
        p_tcp->mode             = RN_TCP_CLIENT;
        p_tcp->opp_port         = p_connect->port;
        strcpy(p_tcp->opp_ip, p_connect->ip);

        //��ʼ���ź���
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
        
        //��ʼ����ʱ��
        p_tcp->p_timer              = new uv_timer_t;
        p_tcp->p_timer->data        = p_tcp;
        uv_timer_init(p_tcp->p_loop, p_tcp->p_timer);
        uv_timer_start(p_tcp->p_timer, on_timer, RN_TIMER_TIMEOUT, RN_TIMER_REPEAT);

        //���سɹ�
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

    //�������У������е������ͳ�ȥ
    while (0 < rj_queue_size(p_client->will_conn_que))
    {
        rn_connect_t *p_connect = (rn_connect_t*)rj_queue_pop_ret(p_client->will_conn_que);

        assert (NULL != p_connect);
        assert (NULL != p_connect->cb);
        assert (NULL != p_connect->obj);
        assert (p_connect->connect.data == p_connect);

        if (0 != uv_ip4_addr(p_connect->ip, p_connect->port, &addr))
        {
            //���ز�������
            p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_PARAM_ERR);
        }

        //��ʼ��TCP����
        p_connect->p_tcp    = new rn_tcp_t;
        assert (NULL != p_connect->p_tcp);
        memset(p_connect->p_tcp, 0, sizeof(rn_tcp_t));

        p_connect->p_tcp->p_loop    = p_client->p_loop;
        p_connect->p_tcp->uv_conn.data = p_connect->p_tcp;
        uv_tcp_init(p_client->p_loop, &p_connect->p_tcp->uv_conn);

        //�������ӷ����
        if (0 != uv_tcp_connect(&p_connect->connect, &p_connect->p_tcp->uv_conn, (const struct sockaddr*)&addr, cb_connect))
        {
            p_connect->cb(NULL, p_connect->obj, p_connect->tag, RN_TCP_TIMEOUT);

            //ʧ�ܣ�Ҫ����TCP����
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
        //��ʧ�ܣ���Ҫ�ر�
        uv_close((uv_handle_t *)&p_server->listen, NULL);

        //����ʧ��
        p_server->cb(NULL, p_server->obj);
    }
    else
    {
        //�ɹ���Ҫ����һ��rn_tcp_conn_t
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

            //��ʼ���ź���
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

            //��ʼ����ʱ��
            p_tcp->p_timer              = new uv_timer_t;
            p_tcp->p_timer->data        = p_tcp;
            uv_timer_init(p_tcp->p_loop, p_tcp->p_timer);
            uv_timer_start(p_tcp->p_timer, on_timer, RN_TIMER_TIMEOUT, RN_TIMER_REPEAT);

            //���سɹ�
            p_server->cb((rn_tcp_h)p_tcp, p_server->obj);
        }
        else
        {
            //�ر�����
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
        //ֹͣ����
        uv_close((uv_handle_t *)&p_server->listen, NULL);
    }
    else
    {
        //��������
        assert (NULL != p_server->cb);
        assert (NULL != p_server->obj);

        p_server->listen.data   = p_server;
        uv_tcp_init(p_server->p_loop, &p_server->listen);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", p_server->port, &addr);
        if (0 != uv_tcp_bind(&p_server->listen, (const struct sockaddr*)&addr, 0))
        {
            //��ʧ�ܣ���Ҫ�رգ��ص�����Ϊ��
            uv_close((uv_handle_t *)&p_server->listen, NULL);

            //����ʧ�ܣ����Ϊ�ձ�ʾ����ʧ�ܣ����Ѿ����ܼ�������
            p_server->cb(NULL, p_server->obj);
        }
        else
        {
            if (0 != uv_listen((uv_stream_t *)(&p_server->listen), 1000, cb_accept))
            {
                //����ʧ�ܣ���Ҫ�رգ��ص�����Ϊ��
                uv_close((uv_handle_t *)&p_server->listen, NULL);

                //����ʧ�ܣ����Ϊ�ձ�ʾ����ʧ�ܣ����Ѿ����ܼ�������
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
        //����һ�����ڿͷ��˵Ķ���
        rn_client_t *p_client = new rn_client_t;
        assert (NULL != p_client);
        memset(p_client, 0, sizeof(rn_client_t));

        p_client->p_loop        = p_loop;

        //����������ڴ���������룬ÿ�����������������ֱ�ӵ���libuv�����ӽӿڣ�
        //���Ƿ���һ��������Ϣ�����źŻص��������ٴӶ���ȡ�����е��������ε���libuv�ӿڽ������ӡ�
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

    //�ر��źţ�Ҫ���ʱlibuv�Ѿ��ر�
    uv_close((uv_handle_t *)&p_client->async, NULL);

    //��������б�
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
        //����һ�����ڷ���˵Ķ���
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

    //�ر��źţ�Ҫ���ʱlibuv�Ѿ��ر�
    uv_close((uv_handle_t *)&p_server->async, NULL);

    //�ر�TCP���ӣ�Ҫ���ʱlibuv�Ѿ��ر�
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
        //�ϲ�Ӧ�õ��øýӿ����뽨�����ӣ�ֻ�Ǵ���һ��p_connect���󣬳�ʼ����Ҫ�Ĳ�����
        //������У�Ȼ����һ�������źŸ�libuv��
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

        //������Ϣ
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

        //������Ϣ
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

        //������Ϣ
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

        //��Ϊ�ͻ��ˣ����ȷ������֣��������ȴ�����
        if (RN_TCP_CLIENT == p_tcp->mode)
        {
            assert (NULL != p_tcp->write.p_async);
            assert (p_tcp == p_tcp->write.p_async->data);
            assert (p_tcp == p_tcp->write.write.data);

            //ֻ�Ƿ���һ���������ݵ��źţ����źŴ���Ե���������ʵ�ʷ�����������
            //����p_tcp->p_tcp_ws�Ƿ�Ϊ�պ������Ƿ�������ж�
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
��������libuv�̵߳��ã�����ֱ�ӵ���libuv�Ľӿ�
��Websocketģʽ�£�Լ����p_data.baseʼ��ָ������ݸ�ʽΪ��rn_ws_packet_t + data��
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

            p_data->len -= sizeof(rn_ws_packet_t);    //��������
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


