//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright(c) 1999-2015, Rejulink Technology LTD, All Rights Reserved
//	Created: 2015/10/3
//
/// @file	 rn_ws_parser.h
/// @brief	 基于TCP封装的WS解释器
///          
/// @author  YSW
/// @version 0.1
/// @history 修改历史
/// @warning 没有警告
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __RN_WS_PARSER_H__
#define __RN_WS_PARSER_H__
#include "rn_tcp.h"

const char  WS_NO_FIN       = 0x00;
const char  WS_IS_FIN       = 0x80;

const char  WS_CONTINUE     = 0x00;
const char  WS_CLOSE        = 0x08;
const char  WS_PING         = 0x09;
const char  WS_PONG         = 0x0A;

#define WS_UN_MASK(buf, key, len) \
    for(size_t i =0; i < len; ++i) { \
       buf[i] = buf[i] ^ key[i & 0x03];\
    }
//////////////////////////////////////////////////////////////////////////

int ws_pack(char *h, char f, char op, char has_k, char* key, uint64 len);

int ws_unpack(char *h, char d_len, char *f, char *op, char *has_k, char *key, uint64 *len);

void ws_rand_key(char *p_buf, int buf_len);

char * ws_get_value(char *p_buf, int buf_len, char *p_str, const char *p_caption);

void ws_encode_key(char *p_dst, char *p_src);

int ws_hs_client(char *p_buf, int buf_len, char *p_key, char *p_protocol, char *p_ip, uint16 port);

int ws_hs_server(char *p_buf, int buf_len, char *p_key, char *p_protocol);

uint32 encryption(char *p_src, uint32 src_len, char *p_dst, uint32 dst_len);

uint32 decrypt(char *p_src, uint32 src_len, char *p_dst, uint32 dst_len);
#endif
//__RN_WS_PARSER_H__
