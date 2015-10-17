#include "util/rj_mem_pool.h"
#include "sys/sys_mem.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>


const int RJ_MAX_MEM_LEN		= 64 * 1024 * 1024;

///	@struct rj_mem_node_t
///	@brief  内存池内存块节点（内部使用）
typedef struct rj_mem_node_t
{
    uint16                  b_locked;                 ///< 内存块的使用状态，0：没有锁住；1：被锁住（不能被分配）。
    uint16                  recv;

    _RJ_MEM_BLOCK_          block;

    rj_mem_node_t           *p_prev_node;
    rj_mem_node_t           *p_next_node;
}rj_mem_node_t;

///	@struct rj_mem_pool_t
///	@brief  内存池（外部使用）
typedef struct rj_mem_pool_t
{
    uint16              block_num;              ///< 内存块总数         
    uint16              total_buf_len;          ///< 内存块总长度（KB）

    char                *p_buf;                 ///< 内存首地址
    rj_mem_node_t       *p_node_list;           ///< 内存块（列表）
}rj_mem_pool_t;

RJ_API rj_mem_pool_h rj_mem_pool_create(char *p_buf, int block_num, int sub_block_len)
{
    //检查参数的有效性，其总长度不能超过RJ_MAX_MEM_LEN

    if ((((0x01 << 12) - 1)  & sub_block_len) && (sub_block_len >= (0x01 << 12)))//块的大于4K,必须为4K的倍数,小于4k不强制要求
        return NULL;

    if (block_num < 2)
        return NULL;

    uint32 mem_len = block_num * sub_block_len;

    if ((0 == mem_len) || (RJ_MAX_MEM_LEN < mem_len))
        return NULL;

    //创建内存池句柄
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)sys_malloc(sizeof(rj_mem_pool_t));//new rj_mem_pool_t;
    assert (NULL != p_mem_pool);

    //分配内存池，改为外部申请内存并传送进来，因为底层使用的时候，可能需要申请一打开物理内存才能高效使用
    //所以不能采用new+delete的方式来处理，至于其实际申请和释放由外部处理。
#if 0
    unsigned char *p_buf = new unsigned char [mem_len];
#endif
    assert (NULL != p_buf);

    //分配失效
    if ((NULL == p_mem_pool) || (NULL == p_buf))
        return NULL;

    //清空数据
    memset(p_mem_pool, 0, sizeof(rj_mem_pool_t));
    memset(p_buf, 0, mem_len);

    //先赋予整块内存的信息
    p_mem_pool->p_buf               = p_buf;
    p_mem_pool->block_num           = block_num;
    p_mem_pool->total_buf_len       = mem_len;

    //建立起内存块的列表
    {
        p_mem_pool->p_node_list = (rj_mem_node_t *)sys_malloc(sizeof(rj_mem_node_t));//new rj_mem_node_t;
        memset(p_mem_pool->p_node_list, 0, sizeof(rj_mem_node_t));

        //第一块内存块的绑定
        p_mem_pool->p_node_list->block.p_buf        = p_mem_pool->p_buf ;
        p_mem_pool->p_node_list->block.buf_len      = sub_block_len;

        int i = 1;
        rj_mem_node_t * p_node = NULL;
        rj_mem_node_t *p_node_prev = p_mem_pool->p_node_list;

        //其他（block_num - 1）块内存块的绑定
        do
        {
            p_node = (rj_mem_node_t *)sys_malloc(sizeof(rj_mem_node_t));//new rj_mem_node_t;
            assert (NULL != p_node);

            memset(p_node, 0, sizeof(rj_mem_node_t));

            //内存块绑定
            p_node->block.p_buf             = p_node_prev->block.p_buf+p_node_prev->block.buf_len;
            p_node->block.buf_len           = sub_block_len;
            
            //节点加入列表
            p_node->p_prev_node             = p_node_prev;
            p_node_prev->p_next_node        = p_node;

            //切换指针，为下一个节点做准备
            p_node_prev = p_node;
            ++ i;
        }while (i < block_num);

        //完成双向列表的对接（必须要有，否则双向列表建立不完整）
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
        //先检查数据的有效性
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        //清除所有的列表节点（new出来的那个部分，数目为p_mem_pool->block_num）
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

        //释放内存池
#if 0
        delete [] p_mem_pool->p_buf;
#endif

        //销毁内存池句柄
        sys_free(p_mem_pool);//delete p_mem_pool;
    }
}

RJ_API _RJ_MEM_BLOCK_ * rj_mem_pool_malloc(rj_mem_pool_h handle)
{
    rj_mem_pool_t   * p_mem_pool = (rj_mem_pool_t *)(handle);

    assert(NULL != p_mem_pool);

    if (NULL != p_mem_pool)
    {
        //先检查数据的有效性
        assert (1 < p_mem_pool->block_num);
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        int i = 0;
        rj_mem_node_t * p_node = p_mem_pool->p_node_list;
        do
        {
            assert (NULL != p_node);
            assert (NULL != p_node->block.p_buf);

            //只支持单线程提取，要是支持多线程提取则需要加锁
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
        //先检查数据的有效性
        assert (1 < p_mem_pool->block_num);
        assert ((0 < p_mem_pool->total_buf_len) || (p_mem_pool->total_buf_len <= RJ_MAX_MEM_LEN));

        int i = 0;
        rj_mem_node_t * p_node = p_mem_pool->p_node_list;

        do
        {
            assert (NULL != p_node);
            assert (NULL != p_node->block.p_buf);

            //依靠内存地址是否相等来判定所指的内存块是否相同，如果有临时扩展和缩减内存块的模式，
            //这种判定就是不安全的。
            if (p_block == &p_node->block)
            {
                assert (1 == p_node->b_locked);

                //申请内存有一个线程（1），使用完可以移交另外一个线程（2），线程（2）可以释放内存
                //即为一个线程申请，一个线程释放的模式，这是安全的。
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