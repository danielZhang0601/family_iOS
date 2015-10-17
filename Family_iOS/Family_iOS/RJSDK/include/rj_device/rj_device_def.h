#ifndef __RJ_DEVICE_DEF_H__
#define __RJ_DEVICE_DEF_H__

#include "pub/rj_frame.h"

#pragma pack(4)


//按位保存，最多不能超过32个。
typedef enum rj_video_size_enum
{
    RJ_VIDEO_SIZE_QCIF			= 0x0001,	//(NTSC 176 X 120),(PAL 176 X 144)
    RJ_VIDEO_SIZE_CIF			= 0x0002,	//(NTSC 352X 240),(PAL 352 X 288)
    RJ_VIDEO_SIZE_NEW_D1		= 0x0004,	//(768 X 432)
    RJ_VIDEO_SIZE_4CIF			= 0x0008,	//(NTSC 704 X 480),(PAL 704 X 576)

    RJ_VIDEO_SIZE_QVGA			= 0x0010,	//(320 X 240)
    RJ_VIDEO_SIZE_VGA			= 0x0020,	//(640 X 480)
    RJ_VIDEO_SIZE_960H			= 0x0040,	//(NTSC 960 X 480),(PAL 960 X 576)
    RJ_VIDEO_SIZE_SVGA			= 0x0080,	//(800 X 600)

    RJ_VIDEO_SIZE_XGA			= 0x0100,	//(1024 X 768)
    RJ_VIDEO_SIZE_XVGA			= 0x0200,	//(1280 X 960)
    RJ_VIDEO_SIZE_SXGA			= 0x0400,	//(1280 X 1024)
    RJ_VIDEO_SIZE_UXGA			= 0x0800,	//(1600 X 1200)

    RJ_VIDEO_SIZE_720P			= 0x1000,	//(1280 X 720)
    RJ_VIDEO_SIZE_1080P		    = 0x2000,	//(1920 X 1080)
    RJ_VIDEO_SIZE_QXGA			= 0x4000,	//(2048 X 1536)
    RJ_VIDEO_SIZE_4K			= 0x8000,	//(3840 x 2160)
}rj_video_size_enum;

/////////////////////////////////////////////////////////////////////////////
#pragma pack()
#endif