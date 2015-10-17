///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//  Created: 2015/08/31
//
/// @file   rj_ndp.h
/// @brief  rj net device protocol RJ网络设备基础协议
///     协议格式 : ws头 + rj_ndp_head_t + data
/// @author xxx
/// @version 0.1
/// @history 修改历史
///     \n xxx 2015/08/31  0.1 创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_NDP_H__
#define __RJ_NDP_H__

#include "util/rj_type.h"
#include "rj_frame.h"

#pragma pack(4)

/// @name  RN_INVALID_TAG
/// @brief 无效的连接id
const int RN_INVALID_TAG     = 0;

/// @name  RN_MAX_PACK_LEN
/// @brief RJ协议的ws包(不含ws头)的最大长度
const int RN_MAX_PACK_LEN       = 65535-16;

/// @name  RJ_NET_MEM_MAX
/// @brief RJ协议网络部分最大的内存大小
const int RN_MAX_NET_MEM_LEN    = 4 * 65536;

/// @enum  rj_ndp_cat_e
/// @brief 分类信息
typedef enum rj_ndp_cat_e
{
    RJNDP_CAT_SESSION           = 0x00,     ///< 会话, json格式
    RJNDP_CAT_EVENT,                        ///< 事件, json流, json格式

    RJNDP_CAT_TALK,                         ///< 音频对讲, 二进制流
    RJNDP_CAT_LIVE,                         ///< 现场流[音视频], 二进制流
    RJNDP_CAT_PLAYBACK,                     ///< 回放流[音视频], 二进制流
    RJNDP_CAT_BACKUP                        ///< 备份流[音视频], 二进制流
}rj_ndp_cat_e;

const char  NDP_PK_NO_FIN   = 0x00;
const char  NDP_PK_FIN      = 0x80;

///	@struct rj_ndp_pk_t
///	@brief  基础协议头, 固定8字节, 小端标准
typedef struct rj_ndp_pk_t
{
    char        fin;                        ///< 是否为完成包
    char        cat;                        ///< 分类(cat <= RJNDP_CAT_TALK)必须为一个分包，不可分包

    //回放、备份的时候，可以把多个通道当成一个流，但是若同时有一个客户端申请两个回放、备份任务，则每个任务用不同的流标识
    //现场时，则需要区分主、子码流，用不同的流标识，检查是否丢包要检查rj_frame_tt中的v_cam_ch。
    uint16      stream_id;                  ///< 流：以一个可以连续解码的视频（中间夹杂音频）为一个流单位。
    uint32      pk_len;                     ///< 数据长度
}rj_ndp_pk_t;

typedef enum live_stream_e
{
    LIVE_STREAM_1   = 0x0000,       ///< 主码流
    LIVE_STREAM_2   = 0x0100,       ///< 子码流

    LIVE_STREAM_PB  = 0x0200,       ///< 回放流的起点
    LIVE_STREAM_BK  = 0x0300        ///< 备份流的起点
}live_stream_e;

typedef struct rn_ws_packet_t
{
    char        resv;
    char        type;           ///< 分包类型，WS_TEXT，WS_BINARY
    char        head[14];       ///< WS头的预留空间
}rn_ws_packet_t;

/// @enum  rjndp_conn_type_e
/// @brief 客户端连接设备的类型
typedef enum rjndp_conn_type_e
{
    RJNDP_CONN_LAN      = 0,                ///< 局域网TCP直连
    RJNDP_CONN_UPNP,                        ///< 公网(UPNP)TCP直连
    RJNDP_CONN_NAT,                         ///< NAT穿透连接
    RJNDP_CONN_RELAY                        ///< 转发连接
}rjndp_conn_type_e;

/// @name  RJ_WS_DEV_PROTOCOL
/// @brief 设备接入的ws子协议
const char RJ_WS_DEV_PROTOCOL[]	= "dev_man_protocol";
#pragma pack()


#endif // __RJ_NDP_H__
//end
