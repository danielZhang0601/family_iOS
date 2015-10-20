//
//  CommonSDK.hpp
//  Family_iOS
//
//  Created by zxd on 15/10/17.
//  Copyright © 2015年 zxd. All rights reserved.
//

#ifndef CommonSDK_hpp
#define CommonSDK_hpp

#include <stdio.h>
#include "net_dev_discov.h"
#include "net_dev_man.h"
#include "sys/sys_pthread.h"
#include "rn_tcp.h"
#include "rn_udp.h"
#include "uv.h"
#include <list>

class CommonSDK
{
public:
    CommonSDK();
    ~CommonSDK();
    
    bool Start();
    void Stop();
    
    int GetDevNetStatus(char *pSN);
    
    void AddDevToSDK(char *buff,int bufLen);
    int GetLocalDev(char *buff,int bufLen);
    
public:
    void LoopRun();
    void DataThreadRun();
    void BroadcastThreadRun();
    
private:
    rn_udp_h        m_udpHandle;
    rn_client_h     m_clientHandle;
    rn_server_h     m_serverHandle;
    dev_discov_h    m_discovHandle;
    net_dev_man_h   m_devManHandle;
    
    uv_loop_t       *m_loop;
    sys_thread_t    *m_loopThread;
    sys_thread_t    *m_dataThread;
    sys_thread_t    *m_broadcaseThread;
    int             m_bLoopRun;
    int             m_bDataRun;
    int             m_bBroadcastRun;
    
//    std::list<discov_dev_t *> m_devList;
    sys_mutex_t *m_pDevLock;
};

#endif /* CommonSDK_hpp */
