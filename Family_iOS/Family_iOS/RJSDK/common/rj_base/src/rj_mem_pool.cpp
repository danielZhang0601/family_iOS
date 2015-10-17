#include "util/rj_mem_pool.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


const int RJ_MAX_MEM_LEN		= 64 * 1024 * 1024;

///	@struct rj_mem_node_t
///	@brief  �ڴ���ڴ��ڵ㣨�ڲ�ʹ�ã�
typedef struct rj_mem_node_t
{
    uint16                  b_locked;                 ///< �ڴ���ʹ��״̬��0��û����ס��1������ס�����ܱ����䣩��
    uint16                  recv;

    _RJ_MEM_BLOCK_          block;

    rj_mem_node_t           *p_prev_node;
    rj_mem_node_t           *p_next_node;
}rj_mem_node_t;

///	@struct rj_mem_pool_t
///	@brief  �ڴ�أ��ⲿʹ�ã�
typedef struct rj_mem_pool_t
{
    uint16              block_num;              ///< �ڴ������         
    uint16              total_buf_len;          ///< �ڴ���ܳ��ȣ�KB��

    char                *p_buf;                 ///< �ڴ��׵�ַ
    rj_mem_node_t       *p_node_list;           ///< �ڴ�飨�б�
}rj_mem_pool_t;

RJ_API rj_mem_pool_h rj_mem_pool_create(char *p_buf, int block_num, int sub_block_len)
{
    //����������Ч�ԣ����ܳ��Ȳ��ܳ���RJ_MAX_MEM_LEN

    if ((((0x01 << 12) - 1)  & sub_block_len) && (sub_block_len >= (0x01 << 12)))//��Ĵ���4K,����Ϊ4K�ı���,С��4k��ǿ��Ҫ��
        return NULL;

    if (block_num < 2)
        return NULL;

    uint32 mem_len = block_num * sub_block_len;

    if ((0 == mem_len) || (RJ_MAX_MEM_LEN < mem_len))
        return NULL;

    //�����ڴ�ؾ��
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)sys_malloc(sizeof(rj_mem_pool_t));//new rj_mem_pool_t;
    assert (NULL != p_mem_pool);

    //�����ڴ�أ���Ϊ�ⲿ�����ڴ沢���ͽ�������Ϊ�ײ�ʹ�õ�ʱ�򣬿�����Ҫ����һ�������ڴ���ܸ�Чʹ��
    //���Բ��ܲ���new+delete�ķ�ʽ������������ʵ��������ͷ����ⲿ����
#if 0
    unsigned char *p_buf = new unsigned char [mem_len];
#endif
    assert (NULL != p_buf);

    //����ʧЧ
    if ((NULL == p_mem_pool) || (NULL == p_buf))
        return NULL;

    //�������
    memset(p_mem_pool, 0, sizeof(rj_mem_pool_t));
    memset(p_buf, 0, mem_len);

    //�ȸ��������ڴ����Ϣ
    p_mem_pool->p_buf               = p_buf;
    p_mem_pool->block_num           = block_num;
    p_mem_pool->total_buf_len       = mem_len;

    //�������ڴ����б�
    {
        p_mem_pool->p_node_list = (rj_mem_node_t *)sys_malloc(sizeof(rj_mem_node_t));//new rj_mem_node_t;
        memset(p_mem_pool->p_node_list, 0, sizeof(rj_mem_node_t));

        //��һ���ڴ��İ�
        p_mem_pool->p_node_list->block.p_buf        = p_mem_pool->p_buf ;
        p_mem_pool->p_node_list->block.buf_len      = sub_block_len;

        int i = 1;
        rj_mem_node_t * p_node = NULL;
        rj_mem_node_t *p_node_prev = p_mem_pool->p_node_list;

        //������block_num - 1�����ڴ��İ�
        do
        {
            p_node = (rj_mem_node_t *)sys_malloc(sizeof(rj_mem_node_t));//new rj_mem_node_t;
            assert (NULL != p_node);

            memset(p_node, 0, sizeof(rj_mem_node_t));

            //�ڴ���
            p_node->block.p_buf             = p_node_prev->block.p_buf+p_node_prev->block.buf_len;
            p_node->block.buf_len           = sub_block_len;
            
            //�ڵ�����б�
            p_node->p_prev_node             = p_node_prev;
            p_node_prev->p_next_node        = p_node;

            //�л�ָ�룬Ϊ��һ���ڵ���׼��
            p_node_prev = p_node;
            ++ i;
        }while (i < block_num);

        //���˫���б�ĶԽӣ�����Ҫ�У�����˫���б�����������
        p_mem_pool->p_node_list->p_prev_node = p_node;
    }

    return reinterpret_cast<rj_mem_pool_h>(p_mem_pool);
}

RJ_API void rj_mem_pool_destroy(rj_mem_pool_h handle)
{
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)(handle);

    assert (NULL != p_mem_pool);
    if (NULL != p_mem_pool)
    {
        //�ȼ�����ݵ���Ч��
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        //������е��б�ڵ㣨new�������Ǹ����֣���ĿΪp_mem_pool->block_num��
        int i = 0;
        rj_mem_node_t * p_next = p_mem_pool->p_node_list;
        rj_mem_node_t * p_node = NULL;
        do
        {
            p_node = p_next;

            assert (NULL != p_node);
            assert ((0 == p_node->b_locked) && (NULL != p_node->block.p_buf));

            p_next = p_node->p_next_node;

            sys_free(p_node);//delete p_node;

            ++ i;
        }while (i < p_mem_pool->block_num);

        //�ͷ��ڴ��
#if 0
        delete [] p_mem_pool->p_buf;
#endif

        //�����ڴ�ؾ��
        sys_free(p_mem_pool);//delete p_mem_pool;
    }
}

RJ_API _RJ_MEM_BLOCK_ * rj_mem_pool_malloc(rj_mem_pool_h handle)
{
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)(handle);

    assert(NULL != p_mem_pool);

    if (NULL != p_mem_pool)
    {
        //�ȼ�����ݵ���Ч��
        assert (1 < p_mem_pool->block_num);
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        int i = 0;
        rj_mem_node_t * p_node = p_mem_pool->p_node_list;
        do
        {
            assert (NULL != p_node);
            assert (NULL != p_node->block.p_buf);

            //ֻ֧�ֵ��߳���ȡ��Ҫ��֧�ֶ��߳���ȡ����Ҫ����
            if (0 == p_node->b_locked)
            {
                p_node->b_locked            = 1;
                p_node->block.data_len      = 0;

                return &p_node->block;
            }

            p_node = p_node->p_next_node;
            ++ i;
        }while (i < p_mem_pool->block_num);
    }
    
    return NULL;
}

RJ_API void rj_mem_pool_free(rj_mem_pool_h handle, _RJ_MEM_BLOCK_ *p_block)
{
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)(handle);

    assert (NULL != p_mem_pool);

    if (NULL != p_mem_pool)
    {
        //�ȼ�����ݵ���Ч��
        assert (1 < p_mem_pool->block_num);
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        int i = 0;
        rj_mem_node_t * p_node = p_mem_pool->p_node_list;

        do
        {
            assert (NULL != p_node);
            assert (NULL != p_node->block.p_buf);

            //�����ڴ��ַ�Ƿ�������ж���ָ���ڴ���Ƿ���ͬ���������ʱ��չ�������ڴ���ģʽ��
            //�����ж����ǲ���ȫ�ġ�
            if (p_block == &p_node->block)
            {
                assert (1 == p_node->b_locked);

                //�����ڴ���һ���̣߳�1����ʹ��������ƽ�����һ���̣߳�2�����̣߳�2�������ͷ��ڴ�
                //��Ϊһ���߳����룬һ���߳��ͷŵ�ģʽ�����ǰ�ȫ�ġ�
                p_node->b_locked        = 0;
                p_block->data_len       = 0;

                break;
            }

            p_node = p_node->p_next_node;
            ++ i;
        }while (i < p_mem_pool->block_num);
    }
}

//end