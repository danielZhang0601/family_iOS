//
//  CommonSDK.cpp
//  Family_iOS
//
//  Created by zxd on 15/10/17.
//  Copyright © 2015年 zxd. All rights reserved.
//

#include "CommonSDK.h"
#include "util/rj_queue.h"
#include "cJSON.h"

const unsigned short BROADCAST_PORT = 12345;



CommonSDK::CommonSDK()
{
    m_udpHandle = NULL;
    m_clientHandle = NULL;
    m_serverHandle = NULL;
    m_discovHandle = NULL;
    m_devManHandle = NULL;
    
    m_loop = NULL;
    m_loopThread = NULL;
    
    m_pDevLock = NULL;
 
}

CommonSDK::~CommonSDK()
{

}

static int loop_routine(void *p_param, int *p_run)
{
    CommonSDK *pSdk = (CommonSDK *)(p_param);
    pSdk->LoopRun();
    return 0;
}

static int data_routine(void *p_param, int *p_run)
{
    CommonSDK *pSdk = (CommonSDK *)(p_param);
    pSdk->DataThreadRun();
    return 0;
}

static int broadcast_routine(void *p_param, int *p_run)
{
    CommonSDK *pSdk = (CommonSDK *)(p_param);
    pSdk->BroadcastThreadRun();
    return 0;
}

bool CommonSDK::Start()
{
    m_loop = new uv_loop_t;
    if(NULL == m_loop)
    {
        goto ST_SDK_ERROR;
    }
    uv_loop_init(m_loop);
    
    m_udpHandle = rn_udp_create(m_loop, BROADCAST_PORT);
    if (NULL == m_udpHandle)
    {
        goto ST_SDK_ERROR;
    }
    
    m_discovHandle = dev_discov_create(m_udpHandle, DD_CLIENT);
    if (NULL == m_discovHandle)
    {
        goto ST_SDK_ERROR;
    }
    
    m_clientHandle = rn_client_create(m_loop);
    if (NULL == m_clientHandle)
    {
        goto ST_SDK_ERROR;
    }
    
    m_serverHandle = rn_server_create(m_loop);
    if (NULL == m_serverHandle)
    {
        goto ST_SDK_ERROR;
    }
    
    m_devManHandle = ndm_create(m_serverHandle, m_clientHandle, 512, 1024*256, 64*1024, 8*1024*1024);
    if (NULL == m_devManHandle)
    {
        goto ST_SDK_ERROR;
    }
    
    m_pDevLock = sys_mutex_create();
    if (NULL == m_pDevLock)
    {
        goto ST_SDK_ERROR;
    }
    
    m_loopThread = sys_thread_create(loop_routine, this, &m_bLoopRun);
    if (NULL == m_loopThread)
    {
        goto ST_SDK_ERROR;
    }
    
    m_dataThread = sys_thread_create(data_routine, this, &m_bDataRun);
    if (NULL == m_dataThread)
    {
        goto ST_SDK_ERROR;
    }
    
    m_broadcaseThread = sys_thread_create(broadcast_routine, this, &m_bBroadcastRun);
    if (NULL == m_broadcaseThread)
    {
        goto ST_SDK_ERROR;
    }
    
    return true;
    
ST_SDK_ERROR:
    Stop();
    return false;
}

void CommonSDK::Stop()
{
    if (NULL != m_loopThread)
    {
        sys_thread_destroy(m_loopThread, &m_bLoopRun);
        m_loopThread = NULL;
    }
    
    if (NULL != m_dataThread)
    {
        sys_thread_destroy(m_dataThread, &m_bDataRun);
        m_dataThread = NULL;
    }
    
    if (NULL != m_broadcaseThread) {
        sys_thread_destroy(m_broadcaseThread, &m_bBroadcastRun);
        m_broadcaseThread = NULL;
    }
    
    if (NULL != m_pDevLock)
    {
        sys_mutex_destroy(m_pDevLock);
        m_pDevLock = NULL;
    }
    
    if (NULL != m_devManHandle)
    {
        ndm_stop(m_devManHandle);
        ndm_destroy(m_devManHandle);
        m_devManHandle = NULL;
    }
    
    if (NULL != m_clientHandle)
    {
        rn_client_destroy(m_clientHandle);
        m_clientHandle = NULL;
    }
    
    if (NULL != m_serverHandle)
    {
        rn_server_destroy(m_serverHandle);
        m_serverHandle = NULL;
    }
    
    if(NULL != m_discovHandle)
    {
        dev_discov_destroy(m_discovHandle);
        m_discovHandle = NULL;
    }
    
    if (NULL != m_udpHandle)
    {
        rn_udp_destroy(m_udpHandle);
        m_udpHandle = NULL;
    }
    
    if (NULL != m_loop)
    {
        uv_loop_close(m_loop);
        delete m_loop;
        m_loop = NULL;
    }
}

void CommonSDK::LoopRun()
{
    while (m_bLoopRun)
    {
        uv_run(m_loop, UV_RUN_DEFAULT);
    }
}

void CommonSDK::DataThreadRun()
{
    
}

void CommonSDK::BroadcastThreadRun()
{
    rj_queue_h devList = rj_queue_create();
    discov_dev_t *pDev = NULL;
    std::list<discov_dev_t *>::iterator iter;
    
    while (m_bBroadcastRun)
    {
        dev_discov_broadcast(m_discovHandle, BROADCAST_PORT);
        
        dev_discov_device(m_discovHandle,devList);
        sys_mutex_lock(m_pDevLock);
        while (rj_queue_size(devList))
        {
            pDev = (discov_dev_t *)rj_queue_pop_ret(devList);
            for (iter = m_devList.begin(); iter != m_devList.end(); iter++)
            {
                if(strcmp(pDev->sn, (*iter)->sn))
                {
                    break;
                }
            }
            
            if (iter == m_devList.end())
            {
                m_devList.push_back(pDev);
            }
            else
            {
                memcpy((*iter), pDev, sizeof(discov_dev_t));
                delete pDev;
            }
        }
        sys_mutex_unlock(m_pDevLock);
        
        sys_sleep(1000);
    }
    
    rj_queue_destroy(devList);
}

int CommonSDK::GetDevNetStatus(const char *pSN)
{
    return 0;
}

void CommonSDK::AddDevToSDK(const char *buff,int bufLen)
{
    
}

int CommonSDK::GetLocalDev(char *buff,int bufLen)
{
    sys_mutex_lock(m_pDevLock);
    
    int ret = 0;
    cJSON *pRoot = cJSON_CreateArray();
    
    for (std::list<discov_dev_t *>::iterator iter = m_devList.begin(); iter != m_devList.end(); iter++)
    {
        cJSON *pItem = cJSON_CreateObject();
        cJSON_AddStringToObject(pItem, "sn", (*iter)->sn);
        cJSON_AddStringToObject(pItem, "addr", (*iter)->addr);
        cJSON_AddStringToObject(pItem, "hard_ver", (*iter)->hard_version);
        cJSON_AddNumberToObject(pItem, "port", (*iter)->port);
        
        cJSON_AddItemToArray(pRoot, pItem);
    }
    
    char *strJSON = cJSON_PrintUnformatted(pRoot);
    ret = strlen(strJSON);
    if (ret < bufLen)
    {
        strcpy(buff, strJSON);
        
    }
    else
    {
        ret = 0;
    }
    
    free(strJSON);
    cJSON_Delete(pRoot);
    
    sys_mutex_unlock(m_pDevLock);
    
    return ret;
}