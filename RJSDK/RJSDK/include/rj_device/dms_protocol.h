//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 2015-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/11
//
/// @file	 dms_protocol.h
/// @brief	 处理DMS协议
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __DMS_PROTOCOL_H__
#define __DMS_PROTOCOL_H__
#include "pub/rj_key.h"
#include "util/rj_type.h"
#include "util/cJSON.h"

int form_online(char *p_buf, int buf_len, uint16 port);

int parse_cmd(cJSON* pRoot, int *p_code, cJSON **pp_data);

int parse_online(cJSON *p_Data, char *p_auth_code, int *p_expired_time);

int parse_res_id(cJSON *p_Data, char *p_IP, uint16 *p_port, char* p_res_id);
#endif
//__DMS_PROTOCOL_H__
