///////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/08/31
//
/// @file	 pub_define.h
/// @brief	 公共常量定义, 勿定义其他信息
/// @author  lsl
/// @version 0.1
/// @history 修改历史
///  \n lsl	2015/08/31	0.1	创建文件
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////
#ifndef __PUB_DEFINE_H__
#define __PUB_DEFINE_H__

/// TODO: 需要考虑内存对齐+多留至少1空位
#define PUB_PADDING_BUF(LEN) (LEN %4 == 0)?(LEN + 4):((LEN + 0x03) &(~0x03))

/// @name  PUB_CAM_NUM
/// @brief 最大视频通道数目
const int PUB_CAM_NUM		= 32;

/// @name  PUB_CAM_NUM
/// @brief 最大视频通道数目
const int PUB_CAM_BUF		= PUB_PADDING_BUF(PUB_CAM_NUM+1);

/// @name  PUB_SN_LEN
/// @brief 设备SN可用长度
const int PUB_DEV_SN_LEN		= 32;

/// @name  PUB_SN_LEN
/// @brief 设备SN存储的缓存大小
const int PUB_DEV_SN_BUF		= PUB_PADDING_BUF(PUB_DEV_SN_LEN);

/// @name  PUB_CID_LEN
/// @brief 客户端标示CID可用长度
const int PUB_CID_LEN		= 32;

/// @name  PUB_CID_BUFF
/// @brief 客户端标示CID的缓存大小
const int PUB_CID_BUF		= PUB_PADDING_BUF(PUB_CID_LEN);

/// @name  PUB_SID_LEN
/// @brief 服务器端SID的可以用长度
const int PUB_SID_LEN		= 32;
/// @name  PUB_SID_BUFF
/// @brief 服务器端SID的缓存大小
const int PUB_SID_BUF		= PUB_PADDING_BUF(PUB_SID_LEN);

/// @name  PUB_IP_LEN
/// @brief IP的可用长度
const int PUB_IP_LEN		= 32;
/// @name  PUB_IP_BUFF
/// @brief IP的缓存大小
const int PUB_IP_BUF		= PUB_PADDING_BUF(PUB_IP_LEN);

/// @name  PUB_MAC_LEN
/// @brief MAC地址的可用长度
const int PUB_MAC_LEN		= 32;

/// @name  PUB_MAC_BUFF
/// @brief MAC地址的缓存大小
const int PUB_MAC_BUF = PUB_PADDING_BUF(PUB_MAC_LEN);

/// @name  PUB_RESOURCE_CODE_LEN
/// @brief 转发服务器分配的资源码的可用长度
const int PUB_RS_RES_CODE_LEN     = 32;

/// @name  PUB_RESOURCE_CODE_BUFF
/// @brief 转发服务器分配的资源码的缓存大小
const int PUB_RS_RES_CODE_BUF	= PUB_PADDING_BUF(PUB_RS_RES_CODE_LEN);

///授权码长度，及缓存长度
const int PUB_AUTH_CODE_LEN = 32;
const int PUB_AUTH_CODE_BUF = PUB_PADDING_BUF(PUB_AUTH_CODE_LEN);

/// @name  PUB_DEVICE_LICENSE
/// @brief 设备的license的可用长度
const int PUB_DEV_LICENSE_LEN	    = 64;

/// @name  PUB_DEVICE_LICENSE_BUFF
/// @brief 设备的license的缓存大小
const int PUB_DEV_LICENSE_BUF	= PUB_PADDING_BUF(PUB_DEV_LICENSE_LEN);

/// @name  PUB_ACCOUNT_LEN
/// @brief 账户名的可用长度
const int PUB_ACCOUNT_LEN   = 32;

/// @name  PUB_ACCOUNT_BUFF
/// @brief 账户名的缓存大小
const int PUB_ACCOUNT_BUF  = PUB_PADDING_BUF(PUB_ACCOUNT_LEN);


/// @name  PUB_ACCOUNT_LEN
/// @brief 密码的可用长度
const int PUB_PASSWORD_LEN   = 32;

/// @name  PUB_ACCOUNT_BUFF
/// @brief 密码的缓存大小
const int PUB_PASSWORD_BUF		= PUB_PADDING_BUF(PUB_PASSWORD_LEN);


/// @name  PUB_CRYPT_PUBLIC_KEY_LEN
/// @brief 加密公钥可用长度
const int PUB_CRYPT_KEY_LEN  = 32;

/// @name  PUB_CRYPT_PUBLIC_KEY_LEN
/// @brief 加密公钥的缓存长度
const int PUB_CRYPT_KEY_BUFF = PUB_PADDING_BUF(PUB_CRYPT_KEY_LEN);


/// @name  PUB_CRYPT_METHOD_LEN
/// @brief 加密方法的可用长度
const int PUB_CRYPT_METHOD_LEN  = 32;

/// @name  PUB_CRYPT_METHOD_BUFF
/// @brief 加密方法的缓存
const int PUB_CRYPT_METHOD_BUF = PUB_PADDING_BUF(PUB_CRYPT_METHOD_LEN);


/// @name  PUB_CRYPT_PASS_LEN
/// @brief 加密私钥的可用长度
const int PUB_CRYPT_PASS_LEN  = 32;

/// @name  PUB_CRYPT_PASS_LEN
/// @brief 加密私钥的可用长度
const int PUB_CRYPT_PASS_BUF  = PUB_PADDING_BUF(PUB_CRYPT_PASS_LEN);


/// @name  PUB_FIRMWARE_VERSION_LEN
/// @brief 软件版本号的长度
const int PUB_FW_VER_LEN   = 32;

/// @name  PUB_FIRMWARE_VERSION_BUFF
/// @brief 软件版本号的buff长度
const int PUB_FW_VER_BUF		= PUB_PADDING_BUF(PUB_FW_VER_LEN);



/// @name  PUB_HARDWARE_VERSION_LEN
/// @brief 硬件版本号的长度
const int PUB_HW_VER_LEN   = 32;

/// @name  PUB_HARDWARE_VERSION_BUFF
/// @brief 硬件版本号的buff长度
const int PUB_HW_VER_BUF		= PUB_PADDING_BUF(PUB_HW_VER_LEN);



/// @name  PUB_EXTRA_DATA_LEN
/// @brief 附加数据的最大长度
const int PUB_EXTRA_DATA_LEN  = 256;

/// @name  PUB_EXTRA_DATA_BUFF
/// @brief 附加数据的buff长度
const int PUB_EXTRA_DATA_BUF  = PUB_PADDING_BUF(PUB_EXTRA_DATA_LEN);

//////////////////////////////////////////
// clients

/// @name  PUB_CLIENT_ID_LEN
/// @brief 客户端ID的最大长度
const int PUB_CLIENT_ID_LEN = 32;

/// @name  PUB_CLIENT_ID_LEN
/// @brief 客户端ID的最大缓存
const int PUB_CLIENT_ID_BUFF  = PUB_PADDING_BUF(PUB_CLIENT_ID_LEN);


/// @name  PUB_SESS_DEV_INFO_LEN
/// @brief 会话的设备信息的最大长度
const int PUB_SESS_DEV_INFO_LEN = 512;

/// @name  PUB_SESS_DEV_INFO_BUFF
/// @brief 会话的设备信息的最大缓存
const int PUB_SESS_DEV_INFO_BUFF = PUB_PADDING_BUF(PUB_SESS_DEV_INFO_LEN);

#endif // __COM_DEFINE_H__
//end
