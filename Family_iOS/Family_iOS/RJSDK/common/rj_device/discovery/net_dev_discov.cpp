#include "net_dev_discov.h"
#include "util/rj_list.h"
#include "sys/sys_pthread.h"
#include "util/cJSON.h"
#include "pub/rj_caption.h"
#include "pub/rj_key.h"
#include <assert.h>
#include <string.h>

const char  DD_BROADCAST    = 0x00;
const char  DD_REPLY        = 0x80;

const char  DD_CLIENT_T[]   = {'C', 'C'};
const char  DD_IPC_T[]      = {'I', 'P'};
const char  DD_NVR_T[]        = {'N', 'R'};

const uint16   DD_MAX_PACK_LEN    = 255;
const uint16   DD_MAX_BUF_LEN     = DD_MAX_PACK_LEN + 4;
const uint16   DD_MAX_DEV_NUM     = 64;




//////////////////////////////////////////////////////////////////////////
typedef struct discov_recv_t
{
    char        fu;
    char        type;
    char        title[2];
    uint16      len;

    uint16      ing_len;
    uv_buf_t    ing;
    sockaddr    addr;
}discov_recv_t;

typedef struct net_dev_discov_t
{
    char            type;
	char			type_t[3];
    
	rn_udp_h        udp_h;
    rj_list_h       dev_que;
	sys_mutex_t		*p_dev_mutex;

    rj_list_h       recving_list;
    uv_buf_t        recv_buf;

    rj_queue_h      send_que;
	sys_mutex_t		*p_send_mutex;
    uv_buf_t        sending;
}net_dev_discov_t;


typedef struct net_reply_t
{
	char fu;					  ///发送类型，有DD_BROADCAST,DD_REPLY两种
	sockaddr addr;                ///发送地址
}net_reply_t;
//////////////////////////////////////////////////////////////////////////

#define DD_PACK(h, fu, type, title, len) { \
	assert (len <= 255);\
	if (len <= 255) {\
	h[0] = (fu | type);\
	h[1] = len;\
	memcpy(h+2, title, 2);}\
}

#define DD_UNPACK(h, len, p_fu, p_type, p_title, p_len) {\
	if ((4 <= len) && (0x00 == (0x70 & h[0])) && (0x00 != (0x0F & h[0]))) {\
	*(p_fu)   = (0x80 & h[0]);\
	*(p_type) = (0x0F & h[0]);\
	*(p_len) = h[1];\
	memcpy(p_title, h+2, 2);}\
	else {\
	*(p_len) = 0;}\
}

#ifdef __RJ_IOS__
char g_dev_sn[PUB_DEV_SN_BUF];
char g_fw_ver[PUB_FW_VER_BUF];
unsigned short g_port;
rj_cap_h g_key_cap;
#else
extern char g_dev_sn[PUB_DEV_SN_BUF];
extern char g_fw_ver[PUB_FW_VER_BUF];
extern unsigned short g_port;
extern rj_cap_h g_key_cap;
#endif


///设备信息JSON格式
/***
{
	"firmware_version":"xxx",     ///设备固件版本号
	"sn":"xxx",               ///设备序列号  
	"port" : N				  ///端口号
}
***/

//////////////////////////////////////////////////////////////////////////
/// @brief 函数描述：创建设备信息JSON格式
/// @param [out]     p_buff:   用于存储JSON数据的缓存
/// @param [in]      buffer_len: 缓存的长度
/// @return int      际JSON字符串的大小
int build_broad_cast(char *p_buff,uint16 buffer_len)
{
	assert(NULL != p_buff);

	cJSON *p_root = cJSON_CreateObject();
	assert(NULL != p_root);
	
	cJSON_AddStringToObject(p_root,rj_caption(g_key_cap,RK_DEV_SN),g_dev_sn);
	cJSON_AddStringToObject(p_root,rj_caption(g_key_cap,RK_DEV_FW_VER),g_fw_ver);
	cJSON_AddNumberToObject(p_root,rj_caption(g_key_cap,RK_PORT),g_port);
	char *p_json_str  = cJSON_PrintUnformatted(p_root);
	assert(NULL != p_json_str);

	int len = strlen(p_json_str);
	if(len < buffer_len)
	{
		strcpy(p_buff,p_json_str);
		p_buff[len] = 0;
		free(p_json_str);
		cJSON_Delete(p_root);
		return len;
	}
	else
	{
		free(p_json_str);
		cJSON_Delete(p_root);
		return 0;
	}
}

//////////////////////////////////////////////////////////////////////////
/// @brief 函数描述：创建设备信息discov_dev_t格式
/// @param [in]      p_json: 网络接收过来的设备信息，JSON格式
/// @param [out]     dev: 保存JSON转结构体后的数据
/// @return bool      true:转换成功，false：转换失败
bool parser_broad_cast(char *p_json,discov_dev_t *p_dev)
{
	if((NULL == p_json) || (NULL == p_dev))
	{
		return false;
	}
	memset(p_dev,0,sizeof(discov_dev_t));

	cJSON *pRoot = cJSON_Parse(p_json);
	if(NULL == pRoot)
	{
		return false;
	}

	cJSON *p_hard_version = cJSON_GetObjectItem(pRoot,rj_caption(g_key_cap,RK_DEV_FW_VER));
	if((NULL == p_hard_version) || (cJSON_String != p_hard_version->type) || (strlen(p_hard_version->valuestring) > PUB_HW_VER_LEN))
	{
		cJSON_Delete(pRoot);
		return false;
	}
	strcpy(p_dev->hard_version,p_hard_version->valuestring);

	cJSON *p_sn = cJSON_GetObjectItem(pRoot,rj_caption(g_key_cap,RK_DEV_SN));
	if((NULL == p_sn) || (cJSON_String != p_sn->type) || (strlen(p_sn->valuestring) > PUB_DEV_SN_LEN))
	{
		cJSON_Delete(pRoot);
		return false;
	}
	strcpy(p_dev->sn,p_sn->valuestring);

	cJSON *p_port = cJSON_GetObjectItem(pRoot,rj_caption(g_key_cap,RK_PORT));
	if(NULL == p_port || cJSON_Number != p_port->type)
	{
		cJSON_Delete(pRoot);
		return false;
	}
	p_dev->port = p_port->valueint;
	cJSON_Delete(pRoot);

	return true;
}

//////////////////////////////////////////////////////////////////////////
void on_send(rn_udp_h udp_h, void *obj, rn_udp_ret_e ret)
{
    if (RN_UDP_OK == ret)
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)obj;
        assert (NULL != p_discov);
        assert (udp_h == p_discov->udp_h);

        if (NULL != p_discov->sending.base)
        {
            delete [] p_discov->sending.base;
            p_discov->sending.base  = NULL;
            p_discov->sending.len   = 0;
        }

		sys_mutex_lock(p_discov->p_send_mutex);
        net_reply_t *p_reply = (net_reply_t *)rj_queue_pop_ret(p_discov->send_que);
		sys_mutex_unlock(p_discov->p_send_mutex);

        if (NULL != p_reply)
        {
			if(DD_CLIENT == p_discov->type)
			{
				p_discov->sending.base  = new char [4];
				p_discov->sending.len = 4;
				//需要补充一个包头
				DD_PACK(p_discov->sending.base, p_reply->fu, p_discov->type, p_discov->type_t, 0);
			}
			else
			{
				p_discov->sending.base = new char[DD_MAX_BUF_LEN];
				p_discov->sending.len = build_broad_cast(p_discov->sending.base+4,DD_MAX_BUF_LEN-4) + 4;
				//需要补充一个包头
				DD_PACK(p_discov->sending.base, p_reply->fu, p_discov->type, p_discov->type_t, p_discov->sending.len-4);
			}
            
            if (RN_UDP_OK != rn_udp_send(p_discov->udp_h, &p_reply->addr, &p_discov->sending))        
            {
                assert (0);
            }
           
			delete p_reply;
        }
    }
}

static void on_alloc(rn_udp_h udp_h, void *obj, ssize_t suggest_size, uv_buf_t *p_buf)
{
    assert (NULL != udp_h);
    assert (NULL != obj);

    if ((NULL != udp_h) && (NULL != obj))
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)obj;
        assert (NULL != p_discov);
        assert (udp_h == p_discov->udp_h);

        p_buf->base     = p_discov->recv_buf.base;
        p_buf->len      = p_discov->recv_buf.len;
    }
    else
    {
        p_buf->base     = NULL;
        p_buf->len      = 0;
    }
}

static discov_recv_t * find_recv(net_dev_discov_t *p_discov, const struct sockaddr * addr)
{
    rj_iterator iter = rj_list_begin(p_discov->recving_list);
    while (iter != rj_list_end(p_discov->recving_list))
    {
        discov_recv_t *p_recv = (discov_recv_t *)rj_iter_data(iter);
        if (0 == memcmp(&p_recv->addr, addr, sizeof(sockaddr)))
        {
            return p_recv;
        }

        iter = rj_iter_add(iter);
    }

    return NULL;
}

static void clean_list(rj_list_h recv_list)
{
    //清理没有数据的
    rj_iterator iter = rj_list_begin(recv_list);
    while (iter != rj_list_end(recv_list))
    {
        discov_recv_t *p_recv = (discov_recv_t *)rj_iter_data(iter);
        if (0 == p_recv->ing_len)
        {
            rj_list_remove(recv_list, iter);
            delete [] p_recv->ing.base;
            delete p_recv;
            return;
        }

        iter = rj_iter_add(iter);
    }

    //清理头都没有收取完整的
    iter = rj_list_begin(recv_list);
    while (iter != rj_list_end(recv_list))
    {
        discov_recv_t *p_recv = (discov_recv_t *)rj_iter_data(iter);
        if (p_recv->ing_len <= 4)
        {
            rj_list_remove(recv_list, iter);

            delete [] p_recv->ing.base;
            delete p_recv;
            return;
        }

        iter = rj_iter_add(iter);
    }

    iter = rj_list_begin(recv_list);
    while (iter != rj_list_end(recv_list))
    {
        discov_recv_t *p_recv = (discov_recv_t *)rj_iter_data(iter);
        if (p_recv->ing_len < p_recv->len)
        {
            rj_list_remove(recv_list, iter);

            delete [] p_recv->ing.base;
            delete p_recv;
            return;
        }

        iter = rj_iter_add(iter);
    }

    //强行清理第一个。
    if (0 < rj_list_size(recv_list))
    {
        discov_recv_t *p_recv = (discov_recv_t *)rj_list_pop_front(recv_list);
        delete [] p_recv->ing.base;
        delete p_recv;
    }
}

static void reply(net_dev_discov_t *p_discov, const struct sockaddr *p_addr)
{
	net_reply_t *p_reply = new net_reply_t;
	p_reply->fu = DD_REPLY;

    memcpy(&p_reply->addr, p_addr, sizeof(sockaddr));
	
	sys_mutex_lock(p_discov->p_send_mutex);
    rj_queue_push(p_discov->send_que, p_reply);
	sys_mutex_unlock(p_discov->p_send_mutex);

    rn_udp_try_write(p_discov->udp_h, on_send, p_discov);
}

static discov_dev_t * find_dev(rj_list_h dev_list, const struct sockaddr *p_addr)
{
    rj_iterator iter = rj_list_begin(dev_list);
    while (iter != rj_list_end(dev_list))
    {
        discov_dev_t *p_dev = (discov_dev_t *)rj_iter_data(iter);
        if (0 == memcmp(&p_dev->addr, p_addr, sizeof(sockaddr)))
        {
            return p_dev;
        }

        iter = rj_iter_add(iter);
    }

    return NULL;
}

static void add_to_list(rj_list_h dev_list, const struct sockaddr* addr, char *p_title, uint16 title_len)
{
    assert ((0 < title_len) && (title_len < DD_MAX_PACK_LEN));

    discov_dev_t *p_dev = find_dev(dev_list, addr);
    if (NULL == p_dev)
    {
		//检查列表是否已经到了最大值
		if (DD_MAX_DEV_NUM <= rj_list_size(dev_list))
		{
			discov_dev_t *p_del = (discov_dev_t *)rj_list_pop_front(dev_list);
			delete p_del;
		}

		p_dev = new discov_dev_t;
    }
    
	if(parser_broad_cast(p_title,p_dev))
	{
		uv_ip4_name((sockaddr_in *)addr,p_dev->addr,PUB_IP_LEN);
		rj_list_push_back(dev_list, p_dev);
	}
	else
	{
		delete p_dev;
	}
}

static int check_data(char *p_title, int type)
{
    if ((DD_CLIENT == type) && (0 == memcmp(p_title, DD_CLIENT_T, 2)))
        return 0;

    if ((DD_IPC == type) && (0 == memcmp(p_title, DD_IPC_T, 2)))
        return 0;

    if ((DD_NVR == type) && (0 == memcmp(p_title, DD_NVR_T, 2)))
        return 0;

    return -1;
}

static void on_recv(rn_udp_h udp_h, void *obj,  ssize_t nread, const uv_buf_t* p_buf, const struct sockaddr* addr)
{
    assert (NULL != udp_h);
    assert (NULL != obj);

    if (nread < 0)
    {
        assert (0);
    }
    else if (nread == 0)
    {

    }
    else
    {
        if ((NULL != udp_h) && (NULL != obj))
        {
            net_dev_discov_t *p_discov = (net_dev_discov_t *)obj;
            assert (NULL != p_discov);
            assert (udp_h == p_discov->udp_h);

            assert (p_buf->base == p_discov->recv_buf.base);

            discov_recv_t *p_recv = find_recv(p_discov, addr);
            if (NULL != p_recv)
            {
                memcpy(p_recv->ing.base + p_recv->ing_len, p_buf->base, nread);
                p_recv->ing_len += nread;
            }
            else    //没有找到，则检查数据合法性后，插入列表
            {
                //检查列表是否已经满了，满了则进行一次清理
                if (DD_MAX_DEV_NUM <= rj_list_size(p_discov->recving_list))
                    clean_list(p_discov->recving_list);

                discov_recv_t   *p_new_recv = new discov_recv_t;
                memset(p_new_recv, 0, sizeof(discov_recv_t));
                p_new_recv->ing.base    = new char [DD_MAX_BUF_LEN];
                p_new_recv->ing.len     = DD_MAX_BUF_LEN;

                memcpy(&p_new_recv->addr, addr, sizeof(sockaddr));
                memcpy(p_new_recv->ing.base, p_buf->base, nread);
                p_new_recv->ing_len     = nread;
                rj_list_push_back(p_discov->recving_list, p_new_recv);

                p_recv  = p_new_recv;
            }

            //若不足一个头部，则继续收取。为了防止积压，当列表到了一定数目，需要对长度不够的进行一次清理。
            if (4 <= p_recv->ing_len)
            {
                DD_UNPACK(p_recv->ing.base, p_recv->ing_len, &p_recv->fu, &p_recv->type, &p_recv->title, &p_recv->len)

                //检查数据合法性
                if (0 != check_data(p_recv->title, p_recv->type))
                {
                    rj_list_remove(p_discov->recving_list, (void *)p_recv);
                    delete [] p_recv->ing.base;
                    delete p_recv;
                }
                else if (p_recv->len <= p_recv->ing_len)//一个包已经能够收取完整
                {
                    //相同类型，则直接丢弃，等到收完整包再丢弃，是为了防止重复产生recv节点并加入列表。
                    if (p_recv->type == p_discov->type)
					{
                        rj_list_remove(p_discov->recving_list, (void *)p_recv);
                        delete [] p_recv->ing.base;
                        delete p_recv;
                    }
                    else 
                    {
                        //客户端广播，则回复。这里不从列表中删除，清理列表时会统一删除。
                        if ((DD_BROADCAST == p_recv->fu) && (DD_CLIENT == p_recv->type) && (DD_CLIENT != p_discov->type))
                            reply(p_discov, addr);

						sys_mutex_lock(p_discov->p_dev_mutex);

						//“IPC广播”或者“回复”，则加入设备列表
						if (((DD_BROADCAST == p_recv->fu) && (DD_IPC == p_recv->type)) || (DD_REPLY == p_recv->fu))
							add_to_list(p_discov->dev_que, addr, p_recv->ing.base+4, p_recv->ing_len-4);   /// 去掉头

						sys_mutex_unlock(p_discov->p_dev_mutex);

                        //如果有多余的数据，则移动到内存前部
                        int free_len = p_recv->ing_len - p_recv->len - 4;
                        if (0 < free_len)
                            memmove(p_recv->ing.base, p_recv->ing.base + p_recv->ing_len, free_len);

                        p_recv->ing_len = free_len;
                    }
                }
                //else继续收取
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////
dev_discov_h dev_discov_create(rn_udp_h udp_h, int type)
{
    assert (NULL != udp_h);

    if (NULL != udp_h)
    {
        net_dev_discov_t *p_discov = new net_dev_discov_t;
        assert (NULL != p_discov);
        p_discov->type              = type;
		if (DD_CLIENT == type)
			strcpy(p_discov->type_t, DD_CLIENT_T);
		else if (DD_IPC == type)
			strcpy(p_discov->type_t, DD_IPC_T);
		else if (DD_NVR == type)
			strcpy(p_discov->type_t, DD_NVR_T);
		else
		{
			delete p_discov;
			return NULL;
		}

        p_discov->udp_h             = udp_h;
        p_discov->dev_que           = rj_list_create();
		p_discov->p_dev_mutex = sys_mutex_create();

        p_discov->recving_list      = rj_list_create();
        p_discov->recv_buf.base     = new char [DD_MAX_BUF_LEN];
        p_discov->recv_buf.len      = DD_MAX_BUF_LEN;

        p_discov->send_que          = rj_queue_create();
		p_discov->p_send_mutex = sys_mutex_create();
        p_discov->sending.base      = NULL;
        p_discov->sending.len       = 0;


		rn_udp_read_start(p_discov->udp_h,on_alloc,on_recv,p_discov);
        return (dev_discov_h)p_discov;
    }

    return NULL;
}

void dev_discov_destroy(dev_discov_h discov_h)
{
    assert (NULL != discov_h);

    if (NULL != discov_h)
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)discov_h;

        if (NULL != p_discov->sending.base)
            delete [] p_discov->sending.base;

        while (0 < rj_queue_size(p_discov->send_que))
        {
            net_reply_t *p_data = (net_reply_t *)rj_queue_pop_ret(p_discov->send_que);
            delete p_data;
        }
        rj_queue_destroy(p_discov->send_que);
		sys_mutex_destroy(p_discov->p_send_mutex);

        if (NULL != p_discov->recv_buf.base)
            delete [] p_discov->recv_buf.base;

        while (0 < rj_list_size(p_discov->recving_list))
        {
            discov_recv_t *p_recv = (discov_recv_t *)rj_list_pop_front(p_discov->recving_list);
            if (NULL != p_recv->ing.base)
                delete [] p_recv->ing.base;

            delete p_recv;
        }
        rj_list_destroy(p_discov->recving_list);

        while (0 < rj_list_size(p_discov->dev_que))
        {
            discov_dev_t *p_dev = (discov_dev_t *)rj_list_pop_front(p_discov->dev_que);
            delete p_dev;
        }
        rj_list_destroy(p_discov->dev_que);
		sys_mutex_destroy(p_discov->p_dev_mutex);

        delete p_discov;
    }
}

void dev_discov_set_addr(dev_discov_h discov_h, char *ip, uint16 port)
{
    assert (NULL != discov_h);
    assert (NULL != ip);

    if ((NULL != discov_h) && (NULL != ip))
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)discov_h;
        assert (NULL != p_discov->udp_h);
        
        //暂时空缺，UDP尚未实现接口
    }
}

void dev_discov_broadcast(dev_discov_h discov_h, uint16 port)
{
    assert (NULL != discov_h);

    if (NULL != discov_h)
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)discov_h;
        assert (NULL != p_discov->udp_h);

        net_reply_t *p_reply = new net_reply_t;
		p_reply->fu = DD_BROADCAST;
        if (0 != uv_ip4_addr("255.255.255.255", port, (sockaddr_in *)(&p_reply->addr)))
        {
            assert(0);
            return;
        }

		sys_mutex_lock(p_discov->p_send_mutex);
        rj_queue_push(p_discov->send_que, p_reply);
		sys_mutex_unlock(p_discov->p_send_mutex);

        if (RN_UDP_OK != rn_udp_try_write(p_discov->udp_h, on_send, p_discov))
            assert (0);
    }
}

void dev_discov_reply(dev_discov_h discov_h, char *ip, uint16 port)
{
    assert (NULL != discov_h);
    assert (NULL != ip);

    if ((NULL != discov_h) && (NULL != ip))
    {
        net_dev_discov_t *p_discov = (net_dev_discov_t *)discov_h;
        assert (NULL != p_discov->udp_h);

		net_reply_t *p_reply = new net_reply_t;
		p_reply->fu = DD_REPLY;

        if (0 != uv_ip4_addr(ip, port, (sockaddr_in *)(&p_reply->addr)))
        {
            assert(0);
            return;
        }

		sys_mutex_lock(p_discov->p_send_mutex);
        rj_queue_push(p_discov->send_que, p_reply);
		sys_mutex_unlock(p_discov->p_send_mutex);

        if (RN_UDP_OK != rn_udp_try_write(p_discov->udp_h, on_send, p_discov))
            assert (0);
    }
}

void dev_discov_device(dev_discov_h discov_h, rj_queue_h device)
{
	assert(NULL != discov_h);
	assert(NULL != device);

	if((NULL != discov_h) && (NULL != device))
	{
		net_dev_discov_t *p_discov = (net_dev_discov_t *)discov_h;
		sys_mutex_lock(p_discov->p_dev_mutex);
		assert(NULL != p_discov->dev_que);
		while(rj_list_size(p_discov->dev_que) > 0)
		{
			rj_queue_push(device,rj_list_pop_front(p_discov->dev_que));
		}
		sys_mutex_unlock(p_discov->p_dev_mutex);
	}
}