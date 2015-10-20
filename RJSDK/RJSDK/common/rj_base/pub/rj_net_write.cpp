#include "pub/rj_net_write.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

RJ_API rj_net_w_h rj_net_write_create(_RJ_MEM_BLOCK_ *p_block)
{
    assert (NULL != p_block);
    assert ((0 < p_block->buf_len) && (p_block->buf_len <= RN_MAX_NET_MEM_LEN));

    if ((NULL != p_block) && (0 < p_block->buf_len) && (p_block->buf_len <= RN_MAX_NET_MEM_LEN))
    {
        p_block->data_len       = 0;

        return (rj_net_w_h)(p_block);
    }

    return NULL;
}

RJ_API _RJ_MEM_BLOCK_ * rj_net_write_destroy(rj_net_w_h handle)
{
    _RJ_MEM_BLOCK_ *p_net_w = (_RJ_MEM_BLOCK_ *)(handle);
    return p_net_w;
}

RJ_API int rj_net_write_push(rj_net_w_h handle, rj_ndp_pk_t *p_ndp, char *p_data, char ws_type)
{
    _RJ_MEM_BLOCK_ *p_net_w = (_RJ_MEM_BLOCK_ *)(handle);

    assert ((NULL != p_net_w) && (NULL != p_ndp) && (NULL != p_data) && (0 < p_ndp->pk_len));

    if ((NULL != p_net_w) && (NULL != p_ndp) && (NULL != p_data) && (0 < p_ndp->pk_len))
    {
        int ws_head_len = 0;
        if (0 <= ws_type)
        {
            ws_head_len = sizeof(rn_ws_packet_t);
            rn_ws_packet_t *p_ws_h = (rn_ws_packet_t *)(p_net_w->p_buf + p_net_w->data_len);
            p_ws_h->type    = ws_type;
        }

        //若条件不满足，则表示没有空间可用了
        uint32  t_data_len = ws_head_len + sizeof(rj_ndp_pk_t) + p_ndp->pk_len;

        if ((p_net_w->data_len + t_data_len) <= p_net_w->buf_len)
        {
            //复制头
            memcpy(p_net_w->p_buf + p_net_w->data_len + ws_head_len, p_ndp, sizeof(rj_ndp_pk_t));
            p_net_w->data_len    += (ws_head_len + sizeof(rj_ndp_pk_t));

            //复制数据
            memcpy(p_net_w->p_buf + p_net_w->data_len, p_data, p_ndp->pk_len);
            p_net_w->data_len    += p_ndp->pk_len;

            return 0;
        }
    }

    return 1;
}

//end
