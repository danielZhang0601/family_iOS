///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/09/07
//
/// @file	 rj_frame.h
/// @brief	 ���������ṹ����
///     ��ʽ: wsͷ + rj_ndp_head_t + rj_ndp_frame_packet_t + rj_frame_t + [h264]
/// @author  xxx
/// @version 0.1
/// @history �޸���ʷ
///  \n xxx	2015/09/07	0.1	�����ļ�
/// @warning û�о���
///////////////////////////////////////////////////////////////////////////
#ifndef __RJ_FRAME_H__
#define __RJ_FRAME_H__


#include "util/rj_type.h"


#pragma pack(4)

const uint32 MAX_FRAME_MEM_LEN = 16*1024*1024;

/// @enum	rj_frame_type_e
/// @brief  ֡����
typedef enum rj_frame_type_e
{
    RJ_FRAME_NULL        = 0x00,    ///< ��Ч����
    RJ_FRAME_AUDIO,                 ///< ��Ƶ
    RJ_FRAME_VIDEO,                 ///< ��Ƶ
    RJ_FRAME_PICTURE                ///< ͼƬ
}rj_frame_type_e;

#define RJ_SET_KEY_FRAME(type) (type |= 0x80)
#define RJ_IS_KEY_FRAME(type) (0x80 == (type&0x80))
#define RJ_FRAME_TYPE(type) (0x7F & (type))

/// @enum  rj_enc_fmt_e
/// @brief ֡�����ʽ
typedef enum rj_enc_fmt_e
{
    RJ_ENC_FMT_H264       = 0x80,     ///< ��Ƶh264��ʽ
    RJ_ENC_FMT_H265       = 0x81,     ///< ��Ƶh265��ʽ

    RJ_ENC_FMT_G711A      = 0x90,     ///< ��Ƶg711.a��ʽ
    RJ_ENC_FMT_G711U      = 0x91,     ///< ��Ƶg711.u��ʽ
    RJ_ENC_FMT_ADPCM      = 0x92,     /// ��Ƶadpcm
    RJ_ENC_FMT_LPCM       = 0x93,     ///��Ƶ�����ʽLPCM
    RJ_ENC_FMT_G726       = 0x94,     ///��Ƶ�����ʽG726
    RJ_ENC_FMT_AAC        = 0x95      ///< ��Ƶaac��ʽ
}rj_enc_fmt_e;

///	@struct rj_frame_t
///	@brief	֡������Ϣ
///	    ����֡��ʽ : rj_frame_t + [h264] + [g711]
typedef struct rj_frame_t
{
    uint8   type;                       ///< ֡����: ö�� rj_frame_type_e
    uint8   enc_fmt;                    ///< ��������: ö�� rj_enc_fmt_e
    
    uint16  v_cam_ch;                   ///< ��Ƶͨ�����

    union{
        struct{
            uint16  witdh;              ///< ��
            uint16  height;             ///< ��
        }video;

        struct{
            uint8   track;              ///< ������: ������=1; ������=2; 5.1����=6;
            uint8   bit_wide;           ///< ����λ��: 8, 16
            uint16  sample;             ///< ������:8K, 44.1 KHz, 48 KHz, 96 KHz, 192 KHz     
        }audio;
    }attr;

    uint32  length;                     ///< ֡����
    uint64  time;                       ///< ʱ���
}rj_frame_t;

#pragma pack()


#endif // __RJ_NDP_FRAME_H__
//end
