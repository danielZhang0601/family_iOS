///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/07
//
/// @file	 rj_frame.h
/// @brief	 二进制流结构描述
///     格式: ws头 + rj_ndp_head_t + rj_ndp_frame_packet_t + rj_frame_t + [h264]
/// @author  xxx
/// @version 0.1
/// @history 修改历史
///  \n xxx	2015/09/07	0.1	创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_FRAME_H__
#define __RJ_FRAME_H__


#include "util/rj_type.h"


#pragma pack(4)

const uint32 MAX_FRAME_MEM_LEN = 16*1024*1024;

/// @enum	rj_frame_type_e
/// @brief  帧类型
typedef enum rj_frame_type_e
{
    RJ_FRAME_NULL        = 0x00,    ///< 无效类型
    RJ_FRAME_AUDIO,                 ///< 音频
    RJ_FRAME_VIDEO,                 ///< 视频
    RJ_FRAME_PICTURE                ///< 图片
}rj_frame_type_e;

#define RJ_SET_KEY_FRAME(type) (type |= 0x80)
#define RJ_IS_KEY_FRAME(type) (0x80 == (type&0x80))
#define RJ_FRAME_TYPE(type) (0x7F & (type))

/// @enum  rj_enc_fmt_e
/// @brief 帧编码格式
typedef enum rj_enc_fmt_e
{
    RJ_ENC_FMT_H264       = 0x80,     ///< 视频h264格式
    RJ_ENC_FMT_H265       = 0x81,     ///< 视频h265格式

    RJ_ENC_FMT_G711A      = 0x90,     ///< 音频g711.a格式
    RJ_ENC_FMT_G711U      = 0x91,     ///< 音频g711.u格式
    RJ_ENC_FMT_ADPCM      = 0x92,     /// 音频adpcm
    RJ_ENC_FMT_LPCM       = 0x93,     ///音频编码格式LPCM
    RJ_ENC_FMT_G726       = 0x94,     ///音频编码格式G726
    RJ_ENC_FMT_AAC        = 0x95      ///< 音频aac格式
}rj_enc_fmt_e;

///	@struct rj_frame_t
///	@brief	帧描述信息
///	    完整帧格式 : rj_frame_t + [h264] + [g711]
typedef struct rj_frame_t
{
    uint8   type;                       ///< 帧类型: 枚举 rj_frame_type_e
    uint8   enc_fmt;                    ///< 编码类型: 枚举 rj_enc_fmt_e
    
    uint16  v_cam_ch;                   ///< 视频通道序号

    union{
        struct{
            uint16  witdh;              ///< 宽
            uint16  height;             ///< 高
        }video;

        struct{
            uint8   track;              ///< 声道数: 单声道=1; 立体声=2; 5.1喇叭=6;
            uint8   bit_wide;           ///< 采样位宽: 8, 16
            uint16  sample;             ///< 采样率:8K, 44.1 KHz, 48 KHz, 96 KHz, 192 KHz     
        }audio;
    }attr;

    uint32  length;                     ///< 帧长度
    uint64  time;                       ///< 时间戳
}rj_frame_t;

#pragma pack()


#endif // __RJ_NDP_FRAME_H__
//end
