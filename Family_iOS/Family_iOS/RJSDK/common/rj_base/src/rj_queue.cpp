#include "util/rj_queue.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

///	@struct rj_queue_node_t
///	@brief  队列节点（内部使用）
typedef struct rj_queue_node_t
{
    void                *p_data;                 ///< 用户数据指针

    rj_queue_node_t     *p_next;                 ///< 后一个节点指针
}rj_queue_node_t;

///	@struct rj_queue_t
///	@brief  队列句柄（外部使用）
typedef struct rj_queue_t
{
    uint32              node_num;           ///< 节点数目

    rj_queue_node_t     *p_head;            ///< 头节点
    rj_queue_node_t     *p_tail;            ///< 尾节点
}rj_queue_t;

RJ_API rj_queue_h rj_queue_create()
{
    rj_queue_t   *p_queue = (rj_queue_t *)sys_malloc(sizeof(rj_queue_t));//new rj_queue_t;
    assert (NULL != p_queue);
    memset(p_queue, 0, sizeof(rj_queue_t));

    return (rj_queue_h)(p_queue);
}

RJ_API void rj_queue_destroy(rj_queue_h handle)
{
    rj_queue_t *p_queue = (rj_queue_t *)(handle);

    assert (NULL != p_queue);
    if (NULL != p_queue)
    {
        //先销毁所有的节点
        uint32 i = 0;
        rj_queue_node_t  *p_node     = NULL;
        rj_queue_node_t  *p_next     = p_queue->p_head;

        while (i < p_queue->node_num)
        {
            p_node  = p_next;
            assert (NULL != p_node);

            p_next  = p_node->p_next;
            sys_free(p_node);//delete  p_node;

            ++ i;
        } 

        //再销毁列表句柄
        sys_free(p_queue);//delete p_queue;
    }
}

RJ_API void rj_queue_push(rj_queue_h handle, void *p_data)
{
    rj_queue_t *p_queue = (rj_queue_t *)(handle);

    assert (NULL != p_queue);
    if (NULL != p_queue)
    {
        rj_queue_node_t  *p_node    = (rj_queue_node_t *)sys_malloc(sizeof(rj_queue_node_t));//new rj_queue_node_t;
        assert (NULL != p_node);
        p_node->p_data      = p_data;
        p_node->p_next      = NULL;

        if (NULL == p_queue->p_tail)
        {
            assert (0 == p_queue->node_num);
            assert (NULL == p_queue->p_head);

            p_queue->p_head     = p_node;
            p_queue->p_tail     = p_node;
        }
        else
        {
            p_queue->p_tail->p_next     = p_node;
            p_queue->p_tail             = p_node;
        }

        ++ p_queue->node_num;
    }
}

RJ_API int rj_queue_pop(rj_queue_h handle, void **pp_data)
{
    rj_queue_t *p_queue = (rj_queue_t *)(handle);

    assert (NULL != p_queue);

    if ((NULL != p_queue) && (0 < p_queue->node_num))
    {
        assert (NULL != p_queue->p_head);

        *pp_data    = p_queue->p_head->p_data;
        rj_queue_node_t  *p_next    = p_queue->p_head->p_next;

        sys_free(p_queue->p_head);//delete  p_queue->p_head;
        p_queue->p_head = p_next;

        if (NULL == p_queue->p_head)
        {
            p_queue->p_tail     = NULL;
        }

        -- p_queue->node_num;

        return 0;
    }

    *pp_data = NULL;
    return 1;
}

RJ_API void * rj_queue_pop_ret(rj_queue_h handle)
{
    rj_queue_t *p_queue = (rj_queue_t *)(handle);

    assert (NULL != p_queue);

    void *p_data = NULL;

    if ((NULL != p_queue) && (0 < p_queue->node_num))
    {
        assert (NULL != p_queue->p_head);

        p_data    = p_queue->p_head->p_data;
        rj_queue_node_t  *p_next    = p_queue->p_head->p_next;

        sys_free(p_queue->p_head);//delete  p_queue->p_head;
        p_queue->p_head = p_next;

        if (NULL == p_queue->p_head)
        {
            p_queue->p_tail     = NULL;
        }

        -- p_queue->node_num;

        return p_data;
    }

    return p_data;
}

RJ_API uint32 rj_queue_size(rj_queue_h handle)
{
    rj_queue_t *p_queue = (rj_queue_t *)(handle);

    assert (NULL != p_queue);
    if (NULL != p_queue)
    {
        return p_queue->node_num;
    }

    return 0;
}

//end
