#include "util/rj_list.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define RJ_ITERATOR_END reinterpret_cast<rj_list_node_t *>(-1)

///	@struct rj_list_node_t
///	@brief  列表节点（内部使用）
typedef struct rj_list_node_t
{
    void                *p_data;                  ///< 用户数据指针
    
    rj_list_node_t      *p_prev;                 ///< 前一个节点指针
    rj_list_node_t      *p_next;                 ///< 后一个节点指针
}rj_list_node_t;

///	@struct rj_list_t
///	@brief  列表句柄（外部使用）
typedef struct rj_list_t
{
    uint32              node_num;           ///< 节点数目

    rj_list_node_t      *p_back;                 ///< 尾节点
    rj_list_node_t      *p_front;                 ///< 头节点
}rj_list_t;

//////////////////////////////////////////////////////////////////////////

rj_iterator rj_iter_add(rj_iterator iter)
{
    assert (NULL != iter);
    rj_list_node_t *p_node = (rj_list_node_t *)(iter);
    return p_node->p_next;
}

rj_iterator rj_iter_dec(rj_iterator iter)
{
    assert (NULL != iter);
    rj_list_node_t *p_node = (rj_list_node_t *)(iter);
    return p_node->p_prev;
}

void * rj_iter_data(rj_iterator iter)
{
    assert (NULL != iter);
    rj_list_node_t *p_node = (rj_list_node_t *)(iter);
    return p_node->p_data;
}

//////////////////////////////////////////////////////////////////////////


RJ_API rj_list_h rj_list_create()
{
    rj_list_t   *p_list = (rj_list_t *)sys_malloc(sizeof(rj_list_t)) ;
    assert (NULL != p_list);
    memset(p_list, 0, sizeof(rj_list_t));

    return (rj_list_h)(p_list);
}

RJ_API void rj_list_destroy(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

	assert (NULL != p_list);
	if (NULL != p_list)
	{

		//先销毁所有的节点
		uint32 i = 0;
		rj_list_node_t  *p_node     = NULL;
		rj_list_node_t  *p_next     = p_list->p_front;

		while (i < p_list->node_num)
		{
			p_node  = p_next;
			assert (NULL != p_node);

			p_next  = p_node->p_next;
            sys_free(p_node);//delete p_node;

			++ i;
		} 

		//再销毁列表句柄
		delete p_list;
	}
}

RJ_API void rj_list_push_front(rj_list_h handle, void *p_data)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert ((NULL != p_list) && (NULL != p_data));

    if ((NULL != p_list) && (NULL != p_data))
    {
        rj_list_node_t  *p_node = (rj_list_node_t *)sys_malloc(sizeof(rj_list_node_t));//new rj_list_node_t;
        assert (NULL != p_node);

        //指向用户数据
        p_node->p_data  = p_data;

        //列表若不为空，则需要把之前的头节点的向前的节点指针指向新的节点
        if (NULL != p_list->p_front)
		{
            p_list->p_front->p_prev = p_node;
			p_node->p_next  = p_list->p_front;
		}
		else
		{
			assert(NULL == p_list->p_back);
			p_list->p_back	= p_node;
			p_node->p_next	= RJ_ITERATOR_END;
		}

        //插入节点到头部
        p_list->p_front     = p_node;

        //衔接无效迭代器指针，用于迭代
        p_node->p_prev  = RJ_ITERATOR_END;
		p_list->node_num++;
    }
}

RJ_API void rj_list_push_back(rj_list_h handle, void *p_data)
{
    rj_list_t   *p_list = reinterpret_cast<rj_list_t *>(handle);

    assert ((NULL != p_list) && (NULL != p_data));

    if ((NULL != p_list) && (NULL != p_data))
    {
        rj_list_node_t  *p_node = (rj_list_node_t *)sys_malloc(sizeof(rj_list_node_t));//new rj_list_node_t;
        assert (NULL != p_node);

        //指向用户数据
        p_node->p_data  = p_data;

        //列表若不为空，则需要把之前的尾节点的向后的节点指针指向新的节点
        if (NULL != p_list->p_back)
		{
            p_list->p_back->p_next = p_node;
			p_node->p_prev  = p_list->p_back;
		}
		else
		{
			assert(NULL == p_list->p_front);
			p_list->p_front	= p_node;
			p_node->p_prev	= RJ_ITERATOR_END;
		}

        //插入节点到尾部
        p_list->p_back     = p_node;

        //衔接无效迭代器指针，用于迭代
        p_node->p_next  = RJ_ITERATOR_END;
		p_list->node_num ++;
    }
}

RJ_API void * rj_list_front(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);
    if (NULL != p_list)
    {
        if (0 < p_list->node_num)
        {
            assert (NULL != p_list->p_front);
            return p_list->p_front->p_data;
        }
    }

    return NULL;
}

RJ_API void * rj_list_back(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);
    if (NULL != p_list)
    {
        if (0 < p_list->node_num)
        {
            assert (NULL != p_list->p_back);
            return p_list->p_back->p_data;
        }
    }

    return NULL;
}

RJ_API void * rj_list_pop_front(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);

    void *p_data    = NULL;

    if (NULL != p_list)
    {
        if (0 < p_list->node_num)
        {
            assert (NULL != p_list->p_front);
            p_data  = p_list->p_front->p_data;

            //把节点删除
            rj_list_node_t  *p_next = p_list->p_front->p_next;
            
            if (RJ_ITERATOR_END != p_next)
            {
				assert (NULL != p_next);
                p_next->p_prev  = p_list->p_back;
            }
            else
			{
				p_list->p_back	= NULL;
			}

            sys_free(p_list->p_front);//delete p_list->p_front;

            p_list->p_front  = (RJ_ITERATOR_END == p_next) ? NULL : p_next;
			p_list->node_num--;
        }
    }

    return p_data;
}

RJ_API void * rj_list_pop_back(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);

    void *p_data    = NULL;

    if (NULL != p_list)
    {
        if (0 < p_list->node_num)
        {
            assert (NULL != p_list->p_back);
            p_data  = p_list->p_back->p_data;

            //把节点删除
            rj_list_node_t  *p_prev = p_list->p_back->p_prev;

            if (RJ_ITERATOR_END != p_prev)
            {
				assert (NULL != p_prev);
                p_prev->p_next  = p_list->p_front;
            }
			else
			{
				p_list->p_front	= NULL;
			}

            sys_free(p_list->p_back);//delete p_list->p_back;

            p_list->p_back  = (RJ_ITERATOR_END == p_prev) ? NULL : p_prev;
			p_list->node_num--;
        }
    }

    return p_data;
}

RJ_API uint32 rj_list_size(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);

    if (NULL != p_list)
        return p_list->node_num;
    else
        return 0;
}

RJ_API rj_iterator rj_list_begin(rj_list_h handle)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    assert (NULL != p_list);

    if (NULL != p_list)
        return (0 >= p_list->node_num) ? RJ_ITERATOR_END : p_list->p_front;
    else
        return RJ_ITERATOR_END;
}

RJ_API rj_iterator rj_list_end(rj_list_h handle)
{
  return  RJ_ITERATOR_END;
}

RJ_API void rj_list_remove(rj_list_h handle, void *p_data)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    if ((NULL != p_list) && (0 < p_list->node_num))
    {
        rj_list_node_t *p_node = p_list->p_front;


        uint32 i = 0, b_delete = 0;

        do 
        {
            assert (NULL != p_node);
            if (p_data == p_node->p_data)
            {
                b_delete = 1;

                rj_list_node_t *p_prev  = p_node->p_prev;
                rj_list_node_t *p_next  = p_node->p_next;
                
                if (RJ_ITERATOR_END == p_prev)
                {
                    if (RJ_ITERATOR_END != p_next)
                    {
                        p_next->p_prev      = RJ_ITERATOR_END;
                        p_list->p_front     = p_next;
                    }
                    else
                    {
                        p_list->p_front     = NULL;
                        p_list->p_back      = NULL;
                    }
                }
                else
                {
                    if (RJ_ITERATOR_END != p_next)
                    {
                        p_prev->p_next  = p_next;
                        p_next->p_prev  = p_prev;
                    }
                    else
                    {
                        p_prev->p_next      = RJ_ITERATOR_END;
                        p_list->p_back     = p_prev;
                    }
                }

                break;
            }

            ++ i;
			p_node = p_node->p_next;

        } while (i < p_list->node_num);

        if (b_delete)
        {
            sys_free(p_node);//delete p_node;
            -- p_list->node_num;
        }
    }
}

void rj_list_remove_iter(rj_list_h handle, rj_iterator iter)
{
    rj_list_t   *p_list = (rj_list_t *)(handle);

    if ((NULL != p_list) && (0 < p_list->node_num))
    {
        assert (NULL != iter);
        rj_list_node_t *p_node = (rj_list_node_t *)(iter);
        rj_list_node_t *p_prev  = p_node->p_prev;
        rj_list_node_t *p_next  = p_node->p_next;

        if (RJ_ITERATOR_END == p_prev)
        {
            if (RJ_ITERATOR_END != p_next)
            {
                p_next->p_prev      = RJ_ITERATOR_END;
                p_list->p_front     = p_next;
            }
            else
            {
                p_list->p_front     = NULL;
                p_list->p_back      = NULL;
            }
        }
        else
        {
            if (RJ_ITERATOR_END != p_next)
            {
                p_prev->p_next  = p_next;
                p_next->p_prev  = p_prev;
            }
            else
            {
                p_prev->p_next      = RJ_ITERATOR_END;
                p_list->p_back     = p_prev;
            }
        }

        sys_free(p_node);//delete p_node;
        -- p_list->node_num;
    }
}
//end
