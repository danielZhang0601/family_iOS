#include "pub/rj_net_read.h"
#include "pub/rj_ndp.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

///	@struct rj_net_read_t
///	@brief  ����д�����ڴ�飨�ⲿʹ�ã�
typedef struct rj_net_read_t
{
    uint16          resv;
    uint16          ws;
    uint32          pos;                ///< ��ǰλ��

    _RJ_MEM_BLOCK_ *p_block;            ///< �ڴ����Ϣ    
}rj_net_read_t;

RJ_API rj_net_r_h rj_net_read_create(_RJ_MEM_BLOCK_ *p_block, int ws)
{
    assert (NULL != p_block);
    assert ((0 < p_block->buf_len) && (p_block->buf_len <= RN_MAX_NET_MEM_LEN));

    if ((NULL != p_block) && (0 < p_block->buf_len) && (p_block->buf_len <= RN_MAX_NET_MEM_LEN))
    {
        rj_net_read_t *p_read = (rj_net_read_t *)sys_malloc(sizeof(rj_net_read_t));//new rj_net_read_t;
        assert (NULL != p_read);

        p_read->ws          = ws;
        p_read->pos         = 0;
        p_read->p_block     = p_block;

        return (rj_net_r_h)(p_read);
    }

    return NULL;
}

RJ_API _RJ_MEM_BLOCK_ * rj_net_read_destroy(rj_net_r_h handle)
{
    rj_net_read_t * p_net_r = (rj_net_read_t *)(handle);

    assert (NULL != p_net_r);
    if (NULL != p_net_r)
    {
        _RJ_MEM_BLOCK_ * p_block = p_net_r->p_block;

        sys_free(p_net_r);//delete  p_net_r;

        return p_block;
    }

    return NULL;
}


RJ_API uint32 rj_net_read_pop(rj_net_r_h handle, char **pp_data)
{
    rj_net_read_t * p_net_r = (rj_net_read_t *)(handle);

    assert (NULL != p_net_r);
    assert (NULL != pp_data);

    if ((NULL != p_net_r) && (NULL != pp_data))
    {
        assert (p_net_r->pos <= p_net_r->p_block->data_len);

        //�����������㣬���ʾ�����Ѿ�����ȡ�꣬�޿���ȡ�����ݡ�
        if (p_net_r->pos < p_net_r->p_block->data_len)
        {
            int ws_head_len = 0;
            if (p_net_r->ws)
                ws_head_len = sizeof(rn_ws_packet_t);

            //ȡ��ͷ��Ϣ�����ݰڷ�Ϊ��rn_ws_packet_t + rj_ndp_head_t + data������(rj_ndp_head_t + data��
            rj_ndp_pk_t *p_head = (rj_ndp_pk_t *)(p_net_r->p_block->p_buf + p_net_r->pos + ws_head_len);
            *pp_data    = p_net_r->p_block->p_buf + p_net_r->pos;

            //ƫ�ƿ�ʼλ�ã���ֹ�ظ���ȡ
            uint32  data_len = ws_head_len + sizeof(rj_ndp_pk_t) + p_head->pk_len;
            p_net_r->pos += data_len;
            assert (p_net_r->pos <= p_net_r->p_block->data_len);

            return data_len;
        }
    }

    *pp_data = NULL;
    return 0;
}